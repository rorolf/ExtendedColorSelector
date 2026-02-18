




using Pkg; Pkg.activate("general", shared=true)
using StaticArrays
using GLMakie

# Kochanek-Bartels-Spline
struct KBSpline{T}
    ItpMat::Matrix{T}
    timepoints::Vector{Float32}
end


# Kochanek-Bartels-Spline
function KBSpline(Ps; ts=range(0,1,length=length(ps)), params=(0,0,-0.3))

    length(Ps) == length(ts) || throw("Constructiong KBSpline:"*
        "Number of points in Space not equal to number of points in Time")
    issorted(ts) || throw("Constructiong KBSpline: points in Time must be in ascending order")
    allunique(ts) || throw("Constructiong KBSpline: points in Time must be unique")

    co, bi, te = bias, tension, continuity = params

    n = length(Ps)

    pdmat = Matrix{eltype(Ps)}(undef, n-1, 4)
    pdmat[1:n-1,1] .= view(Ps,1:n-1)
    pdmat[1:n-1,2] .= view(Ps,2:n)
    #pdmat[end,2] = 2*Ps[n]-Ps[n-1]

    DDs = view(pdmat,1:n-1,3) # pdmat[1:end,3] .= DDs
    DSs = view(pdmat,1:n-1,4) # pdmat[1:end,4] .= DSs

    Df1 = 0.5*(1-te)*(1+co)*(1+bi)
    Df2 = 0.5*(1-te)*(1-co)*(1-bi)

    DDs[1] = (Df1+Df2)*Ps[2]-(Df1+Df2)*Ps[1] # Ps[0] = 2*Ps[1]-Ps[2]
    for k in 2:n-1
        DDs[k] = Df2*Ps[k+1]+(Df1-Df2)*Ps[k]-Df1*Ps[k-1]
    end
    #DDs[n] = 2*Df2*Ps[n]+(Df1-Df2)*Ps[n]-(Df1+Df2)*Ps[n-1] # Ps[n+1] = 2*Ps[n]-Ps[n-1]

    Sf1 = 0.5*(1-te)*(1-co)*(1+bi)
    Sf2 = 0.5*(1-te)*(1+co)*(1-bi)

    #DSs[1] = (Sf1+Sf2)*Ps[2]-(Sf1+Sf2)*Ps[1] # Ps[0] = 2*Ps[1]-Ps[2]
    for k in 2:n-1
        DSs[k-1] = Sf2*Ps[k+1]+(Sf1-Sf2)*Ps[k]-Sf1*Ps[k-1]
    end
    DSs[n-1] = 2*Sf2*Ps[n]+(Sf1-Sf2)*Ps[n]-(Sf1+Sf2)*Ps[n-1] # Ps[n+1] = 2*Ps[n]-Ps[n-1]


    hermite = [1 -1 0 0; 1 -2 1 0; -2 3 0 0; 2 -3 0 1]
    m = permutedims(pdmat)*hermite

    hermite = [2 -2 1 1; -3 3 -2 -1; 0 0 1 0; 1 0 0 0]

    m = hermite * pdmat

    return KBSpline(m, Float32.(ts))
end

function (kbs::KBSpline)(t::TF) where TF<:Real
    mint, maxt = kbs.timepoints[1], kbs.timepoints[end]
    t = clamp(t, mint, maxt)
    tindex = findlast(<=(t), kbs.timepoints) # cannot be nothing
    tlow = kbs.timepoints[tindex]
    thigh = if tindex<length(kbs.timepoints)
            tlow+1
        else
            2*kbs.timepoints[end]-kbs.timepoints[end-1]
    end
    t = (t-tlow)/(thigh-tlow)
    poly_base = (one(t), t, t*t, t*t*t)

    #res = kbs.ItpMat |> first |> zero
    r1 = r2 = r3 = zero(Float32)
    for k1 in 1:4
        v = kbs.ItpMat[k1, tindex]
        r1 += v[1]*poly_base[k1]
        r2 += v[2]*poly_base[k1]
        r3 += v[3]*poly_base[k1]
    end

    return Point3f(r1,r2,r3)
end











