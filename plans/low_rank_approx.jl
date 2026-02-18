

using LinearAlgebra

# --- basic tensor utilities (3D only) ---

# mode-n unfolding
function unfold(T::Array{<:Real,3}, mode::Int)
    I, J, K = size(T)
    if mode == 1
        return reshape(T, I, J*K)
    elseif mode == 2
        # permute (2,1,3) then reshape
        return reshape(permutedims(T, (2,1,3)), J, I*K)
    elseif mode == 3
        return reshape(permutedims(T, (3,1,2)), K, I*J)
    else
        error("mode must be one of 1,2 or 3")
    end
end

# mode-n product: (U * unfold(T,n)) folded back
function mode_product(T::Array{Float64,3}, A::Matrix{Float64}, mode::Int)
    I, J, K = size(T)
    if mode == 1
        X = A * unfold(T, 1)                  # (R×I) * (I×JK) => R×JK
        R = size(A, 1)
        return reshape(X, R, J, K)
    elseif mode == 2
        X = A * unfold(T, 2)                  # (R×J) * (J×IK) => R×IK
        R = size(A, 1)
        X3 = reshape(X, R, I, K)              # R×I×K in permuted space
        return permutedims(X3, (2,1,3))       # I×R×K
    elseif mode == 3
        X = A * unfold(T, 3)                  # (R×K) * (K×IJ) => R×IJ
        R = size(A, 1)
        X3 = reshape(X, R, I, J)              # R×I×J
        return permutedims(X3, (2,3,1))       # I×J×R
    else
        error("mode must be one of 1,2 or 3")
    end
end

# HOSVD Tucker fit
struct Tucker3
    U::Matrix{Float64}   # I×r1
    V::Matrix{Float64}   # J×r2
    W::Matrix{Float64}   # K×r3
    G::Array{Float64,3}  # r1×r2×r3
end

function tucker_hosvd(T::Array{<:Real,3}, r1::Int, r2::Int, r3::Int)
    T64 = Array{Float64,3}(T)

    U = svd(unfold(T64, 1)).U[:, 1:r1]
    V = svd(unfold(T64, 2)).U[:, 1:r2]
    W = svd(unfold(T64, 3)).U[:, 1:r3]

    # core: G = T ×1 U' ×2 V' ×3 W'
    G = mode_product(mode_product(mode_product(T64, U|>permutedims, 1), V|>permutedims, 2), W|>permutedims, 3)
    return Tucker3(U, V, W, G)
end

# reconstruct full tensor (for debugging/error measurement)
function reconstruct(model::Tucker3)
    T = mode_product(mode_product(mode_product(model.G, model.U, 1), model.V, 2), model.W, 3)
    return T
end


# 1D linear interpolation into a factor matrix rows.
# M is N×r. x in [0,1] maps across row index 1..N.
function interp_rows(M::Matrix{Float64}, x::Float64)
    N, r = size(M)
    t = clamp(x, 0.0, 1.0) * (N - 1) + 1.0
    i0 = Int(floor(t))
    i1 = min(i0 + 1, N)
    α = t - i0
    @inbounds return (1 - α) .* view(M, i0, :) .+ α .* view(M, i1, :)
end

# Evaluate Tucker model at continuous coordinates (u,v,w in [0,1])
function evaluate(model::Tucker3, u::Float64, v::Float64, w::Float64)
    a = interp_rows(model.U, u)   # length r1
    b = interp_rows(model.V, v)   # length r2
    c = interp_rows(model.W, w)   # length r3

    G = model.G
    r1, r2, r3 = size(G)

    # Contract: sum_{i,j,k} G[i,j,k]*a[i]*b[j]*c[k]
    s = 0.0
    @inbounds for k in 1:r3
        ck = c[k]
        for j in 1:r2
            bjck = b[j] * ck
            for i in 1:r1
                s += G[i,j,k] * a[i] * bjck
            end
        end
    end
    return s
end

using LinearAlgebra

struct CP3
    A::Matrix{Float64}  # I×r
    B::Matrix{Float64}  # J×r
    C::Matrix{Float64}  # K×r
end

# Khatri-Rao product of two matrices with same column count: (X ⊙ Y)
# returns (size(X,1)*size(Y,1)) × r
function khatri_rao(X::Matrix{Float64}, Y::Matrix{Float64})
    nX, r = size(X)
    nY, r2 = size(Y)
    r == r2 || error("column mismatch")
    Z = Matrix{Float64}(undef, nX*nY, r)
    @inbounds for p in 1:r
        col = kron(view(X, :, p), view(Y, :, p))
        Z[:, p] = col
    end
    return Z
end

# Matricized Tensor Times Khatri-Rao Product (MTTKRP) for 3D
function mttkrp(T::Array{Float64,3}, mode::Int, X::Matrix{Float64}, Y::Matrix{Float64})
    # returns M = unfold(T, mode) * (X ⊙ Y)
    M = unfold(T, mode) * khatri_rao(X, Y)
    return M
end

# Initialize CP from Tucker factors (often stabilizes ALS)
function cp_init_from_hosvd(T::Array{<:Real,3}, r::Int)
    t = tucker_hosvd(T, r, r, r)
    return CP3(copy(t.U), copy(t.V), copy(t.W))
end

function cp_als(T::Array{<:Real,3}, r::Int; maxiter::Int=50, tol::Float64=1e-6, init=:hosvd)
    T64 = Array{Float64,3}(T)
    I, J, K = size(T64)

    model = init == :hosvd ? cp_init_from_hosvd(T64, r) :
            CP3(randn(I,r), randn(J,r), randn(K,r))

    A, B, C = model.A, model.B, model.C

    # Precompute tensor norm for relative error reporting
    normT = norm(vec(T64))

    last_err = Inf
    for it in 1:maxiter
        # Update A: A = MTTKRP(mode=1, B, C) * pinv((B'B) .* (C'C))
        BtB = B' * B
        CtC = C' * C
        G = BtB .* CtC
        A = mttkrp(T64, 1, C, B) * pinv(G)

        AtA = A' * A
        G = AtA .* CtC
        B = mttkrp(T64, 2, C, A) * pinv(G)

        BtB = B' * B
        G = AtA .* BtB
        C = mttkrp(T64, 3, B, A) * pinv(G)

        # Optional: normalize columns to avoid scale blowup
        for p in 1:r
            s = (norm(view(A,:,p)) * norm(view(B,:,p)) * norm(view(C,:,p)))^(1/3)
            s = s == 0 ? 1.0 : s
            A[:,p] ./= s
            B[:,p] ./= s
            C[:,p] ./= s
        end

        # Compute a cheap-ish relative error by reconstructing (ok for prototype)
        T̂ = zeros(Float64, I, J, K)
        @inbounds for p in 1:r
            ap = view(A,:,p); bp = view(B,:,p); cp = view(C,:,p)
            for k in 1:K, j in 1:J, i in 1:I
                T̂[i,j,k] += ap[i] * bp[j] * cp[k]
            end
        end
        err = norm(vec(T64 .- T̂)) / normT

        if abs(last_err - err) < tol
            break
        end
        last_err = err
    end

    return CP3(A, B, C)
end

function cp_als_stable(T::Array{<:Real,3}, r::Int;
                       maxiter::Int=50, tol::Float64=1e-6,
                       λ::Float64=1e-8, init=:hosvd)

    T64 = Array{Float64,3}(T)
    I, J, K = size(T64)

    model = init == :hosvd ? cp_init_from_hosvd(T64, r) :
            CP3(randn(I,r), randn(J,r), randn(K,r))

    A, B, C = model.A, model.B, model.C
    normT = norm(vec(T64))

    last_err = Inf
#     Id = Matrix{Float64}(I, r, r)  # r×r identity
#     fill!(Id, 0.0)
#     @inbounds for p in 1:r
#         Id[p,p] = 1.0
#     end
#
    #Id = LinearAlgebra.I
    Id = [ifelse(x==y, 1.0, 0.0) for y in 1:r,x in 1:r]

    for it in 1:maxiter
        # --- Update A ---
        BtB = B' * B
        CtC = C' * C
        G = BtB .* CtC .+ λ .* Id
        M = mttkrp(T64, 1, C, B)              # I×r
        A = M / G                             # solve (A*G = M)

        # --- Update B ---
        AtA = A' * A
        CtC = C' * C
        G = AtA .* CtC .+ λ .* Id
        M = mttkrp(T64, 2, C, A)              # J×r
        B = M / G

        # --- Update C ---
        AtA = A' * A
        BtB = B' * B
        G = AtA .* BtB .+ λ .* Id
        M = mttkrp(T64, 3, B, A)              # K×r
        C = M / G

        # Column normalization (keeps scales under control)
        @inbounds for p in 1:r
            na = norm(view(A,:,p)); nb = norm(view(B,:,p)); nc = norm(view(C,:,p))
            s = (na * nb * nc)^(1/3)
            if s > 0
                A[:,p] ./= s
                B[:,p] ./= s
                C[:,p] ./= s
            end
        end

        # Compute relative Frobenius error (prototype: reconstruct)
        T̂ = reconstruct(CP3(A,B,C))
        err = norm(vec(T64 .- T̂)) / (normT == 0 ? 1.0 : normT)

        if abs(last_err - err) < tol
            break
        end
        last_err = err
    end

    return CP3(A,B,C)
end


# Continuous evaluation for CP (interpolate factors and dot-product)
function evaluate(model::CP3, u::Float64, v::Float64, w::Float64)
    a = interp_rows(model.A, u)
    b = interp_rows(model.B, v)
    c = interp_rows(model.C, w)
    # sum_p a[p]*b[p]*c[p]
    s = 0.0
    @inbounds for p in eachindex(a)
        s += a[p] * b[p] * c[p]
    end
    return s
end



I, J, K = 33, 33, 33

function create_data(I=33,J=I,K=J)
    T = Array{Float64,3}(undef, I, J, K)
    for k in 1:K, j in 1:J, i in 1:I
        x = (i-1)/(I-1); y = (j-1)/(J-1); z = (k-1)/(K-1)
        T[i,j,k] = sin(2π*x) * cos(2π*y) + 0.3*z + 0.1*sin(4π*z)
    end
    return T
end

function comparison(data=create_data())
    T = if data isa Function
            data()
        else
            data
    end
    tuck = tucker_hosvd(T, 6, 6, 6)
    cp   = cp_als(T, 8, maxiter=30)
    #cp   = cp_als_stable(T, 8, maxiter=30)

    println()
    println(evaluate(tuck, 0.12, 0.34, 0.56))
    println(evaluate(cp,   0.12, 0.34, 0.56))

    s1,s2,s3 = size(T)
    err_tuck = 0.0; err_tuck_max = 0.0;
    err_cp = 0.0; err_cp_max = 0.0;
    @inbounds for ci in CartesianIndices(T)
        i,j,k = ci.I
        x,y,z = ((i,j,k) .-1) ./ (size(T).-1)
        err_tuck_now = abs(T[i,j,k]-evaluate(tuck, x,y,z))
        err_tuck += err_tuck_now
        err_tuck_max = max(err_tuck_max, err_tuck_now)
        err_cp_now   = abs(T[i,j,k]-evaluate(cp,   x,y,z))
        err_cp  += err_cp_now
        err_cp_max = max(err_cp_max, err_cp_now)
    end

    sT = Base.summarysize(T)
    println("Tucker compression: ", sT=>Base.summarysize(tuck))
    println("CP     compression: ", sT=>Base.summarysize(cp))
    println("Tucker Error overall: ", err_tuck, " and maximum: ", err_tuck_max)
    println("CP     Error overall: ", err_cp,   " and maximum: ", err_cp_max)

    st_t = stats(T, tuck)
    st_c = stats(T, cp)

    println("\n--- Tucker ---")
    print_stats(st_t)

    println("\n--- CP ---")
    print_stats(st_c)

end



using LinearAlgebra

# ----------------------------
# Parameter counts / compression
# ----------------------------

"Number of Float64 parameters stored by the Tucker model."
function nparams(model::Tucker3)
    I, r1 = size(model.U)
    J, r2 = size(model.V)
    K, r3 = size(model.W)
    return I*r1 + J*r2 + K*r3 + r1*r2*r3
end

"Number of Float64 parameters stored by the CP model."
function nparams(model::CP3)
    I, r = size(model.A)
    J, r2 = size(model.B)
    K, r3 = size(model.C)
    r == r2 == r3 || error("CP factor column mismatch")
    return I*r + J*r + K*r
end

"Original tensor element count."
nelems(T::Array{<:Real,3}) = prod(size(T))

"Compression ratio: original floats / stored parameter floats."
compression_ratio(T::Array{<:Real,3}, model) = nelems(T) / nparams(model)

"Byte sizes (approx, in-memory), assuming Float64."
bytes_original(T::Array{<:Real,3}) = nelems(T) * sizeof(Float64)
bytes_model(model) = nparams(model) * sizeof(Float64)

# ----------------------------
# Reconstructors (for stats)
# ----------------------------

function reconstruct(model::CP3)
    I, r = size(model.A)
    J, r2 = size(model.B)
    K, r3 = size(model.C)
    r == r2 == r3 || error("CP factor column mismatch")

    T̂ = zeros(Float64, I, J, K)
    @inbounds for p in 1:r
        ap = view(model.A, :, p)
        bp = view(model.B, :, p)
        cp = view(model.C, :, p)
        for k in 1:K, j in 1:J, i in 1:I
            T̂[i,j,k] += ap[i] * bp[j] * cp[k]
        end
    end
    return T̂
end

# Tucker reconstruct(model::Tucker3) already defined earlier in my prototype.
# If you used the same name, keep only one in your file.

# ----------------------------
# Error statistics
# ----------------------------

struct ApproxStats
    original_size::NTuple{3,Int}
    n_original_floats::Int
    n_model_params::Int
    compression_ratio::Float64
    original_bytes::Int
    model_bytes::Int

    rel_fro_error::Float64     # ||T - T̂||_F / ||T||_F
    rmse::Float64              # sqrt(mean((T-T̂)^2))
    max_abs_error::Float64     # maximum |T - T̂|
    max_rel_error::Float64     # maximum |T - T̂| / max(|T|, eps)
    argmax_abs::CartesianIndex{3}
end

"Compute stats by reconstructing the full approximation."
function stats(T::Array{<:Real,3}, model; eps::Float64 = 1e-12)
    T64 = Array{Float64,3}(T)
    T̂  = reconstruct(model)

    E = T64 .- T̂

    nT = norm(vec(T64))
    nE = norm(vec(E))
    relF = nT == 0 ? (nE == 0 ? 0.0 : Inf) : (nE / nT)

    rmse = sqrt(sum(abs2, E) / length(E))

    absE = abs.(E)
    max_abs, linidx = findmax(vec(absE))
    argmax_abs = CartesianIndices(T64)[linidx]

    # pointwise relative error with guard
    denom = max.(abs.(T64), eps)
    max_rel = maximum(absE ./ denom)

    n_orig = nelems(T64)
    n_par  = nparams(model)
    cr     = n_orig / n_par

    return ApproxStats(
        size(T64),
        n_orig,
        n_par,
        cr,
        bytes_original(T64),
        bytes_model(model),
        relF,
        rmse,
        max_abs,
        max_rel,
        argmax_abs
    )
end

function print_stats(s::ApproxStats)
    println("Tensor size:               ", s.original_size)
    println("Original floats:           ", s.n_original_floats)
    println("Model params (floats):     ", s.n_model_params)
    println("Compression ratio:         ", round(s.compression_ratio, digits=3), "×  (floats)")
    println("Original bytes (Float64):  ", s.original_bytes)
    println("Model bytes (Float64):     ", s.model_bytes)
    println("Relative Frobenius error:  ", s.rel_fro_error)
    println("RMSE:                      ", s.rmse)
    println("Max abs error:             ", s.max_abs_error, " at ", s.argmax_abs)
    println("Max rel error (guarded):   ", s.max_rel_error)
end
