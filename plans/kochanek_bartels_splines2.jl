


#module KBSplines


using Pkg; Pkg.activate("general", shared=true)
using GLMakie              # provides Point3f (alias of GeometryBasics.Point{3,Float32})
# If you prefer avoiding GLMakie, replace Point3f with GeometryBasics.Point3f0 and `using GeometryBasics`

const P3 = Point3f

"""
    KBSpline(points, times; tension=0, continuity=0, bias=0)

Kochanek–Bartels (T,C,B) spline through `points` at keyframe `times`.

- `points :: Vector{Point3f}`
- `times  :: AbstractVector{<:Real}` (strictly increasing)

Precomputes per-segment cubic polynomial coefficients so evaluation is fast.
The spline object is callable: `s(tquery)` returns a `Point3f`.
"""
struct KBSpline
    times  :: Vector{Float32}     # keyframe times t0..tn
    # per segment: (a,b,c,d) with C(u)=a*u^3 + b*u^2 + c*u + d, u∈[0,1]
    coeffs :: Vector{NTuple{4,P3}}
    T      :: Float32
    C      :: Float32
    B      :: Float32
end

# Small helpers
@inline _toP3(v) = P3(v[1], v[2], v[3])
@inline _sub(a::P3, b::P3) = P3(a[1]-b[1], a[2]-b[2], a[3]-b[3])
@inline _add(a::P3, b::P3) = P3(a[1]+b[1], a[2]+b[2], a[3]+b[3])
@inline _mul(s::Real, a::P3) = P3(Float32(s)*a[1], Float32(s)*a[2], Float32(s)*a[3])

# Mirror endpoint accessor for points/times (gives you P[-1], t[-1] and P[n+1], t[n+1] implicitly)
function _pmir(points::Vector{P3}, times::Vector{Float32}, i::Int)
    n = length(points)
    if 1 ≤ i ≤ n
        return points[i], times[i]
    elseif i == 0
        # mirror left: P[-1] = 2P0 - P1, t[-1] = 2t0 - t1 (1-based indexing => i=0 means "(-1)")
        P0, t0 = points[1], times[1]
        P1, t1 = points[2], times[2]
        return _sub(_mul(2, P0), P1), Float32(2t0 - t1)
    elseif i == n+1
        # mirror right: P[n+1] = 2Pn - P[n-1], t[n+1] = 2tn - t[n-1]
        Pn, tn = points[n], times[n]
        Pm, tm = points[n-1], times[n-1]
        return _sub(_mul(2, Pn), Pm), Float32(2tn - tm)
    else
        throw(BoundsError("Only mirroring for i=0 and i=n+1 is supported. Got i=$i."))
    end
end

"""
Compute outgoing (m⁺) and incoming (m⁻) tangents (derivatives w.r.t *time*) at keyframe i.
Kochanek–Bartels parameters:
- T: tension
- C: continuity
- B: bias
"""
function _kb_tangents_at(points::Vector{P3}, times::Vector{Float32},
                         i::Int, T::Float32, C::Float32, B::Float32)
    # We want P_{i-1},P_i,P_{i+1} with mirrored endpoints
    (Pim1, tim1) = _pmir(points, times, i-1)
    (Pi,   ti)   = _pmir(points, times, i)
    (Pip1, tip1) = _pmir(points, times, i+1)

    # Secants (derivative estimates) on each side, w.r.t *time*
    dt0 = ti   - tim1
    dt1 = tip1 - ti
    dt0 == 0f0 && throw(ArgumentError("Duplicate time at i=$i (dt0=0)."))
    dt1 == 0f0 && throw(ArgumentError("Duplicate time at i=$i (dt1=0)."))

    d0 = _mul(1/dt0, _sub(Pi, Pim1))
    d1 = _mul(1/dt1, _sub(Pip1, Pi))

    # KB weights (classic form)
    k = 1f0 - T

    # outgoing tangent at Pi (m_i^+)
    w0p = k * (1f0 + C) * (1f0 + B) * 0.5f0
    w1p = k * (1f0 - C) * (1f0 - B) * 0.5f0
    mp  = _add(_mul(w0p, d0), _mul(w1p, d1))

    # incoming tangent at Pi (m_i^-)
    w0m = k * (1f0 - C) * (1f0 + B) * 0.5f0
    w1m = k * (1f0 + C) * (1f0 - B) * 0.5f0
    mm  = _add(_mul(w0m, d0), _mul(w1m, d1))

    return mp, mm
end

# Multiply Hermite matrix by geometry vector [P0, P1, M0, M1] (component-wise)
function _hermite_coeffs(P0::P3, P1::P3, M0::P3, M1::P3)
    # --- Hermite cubic basis matrix (a.k.a. "Hermitian matrix") ---
    # Given geometry G = [P0, P1, M0, M1] (columns), coefficients are:
    # [a; b; c; d] = H * [P0; P1; M0; M1]  (component-wise)
    HERMITE_BASE_H = Float32[
        2  -2   1   1;
        -3   3  -2  -1;
        0   0   1   0;
        1   0   0   0
    ]
    # We compute rows of H * [P0,P1,M0,M1] without allocating 4x4 intermediates.
    # Each coeff is a Point3f.
    a = _add(_add(_mul(HERMITE_BASE_H[1,1], P0), _mul(HERMITE_BASE_H[1,2], P1)),
             _add(_mul(HERMITE_BASE_H[1,3], M0), _mul(HERMITE_BASE_H[1,4], M1)))
    b = _add(_add(_mul(HERMITE_BASE_H[2,1], P0), _mul(HERMITE_BASE_H[2,2], P1)),
             _add(_mul(HERMITE_BASE_H[2,3], M0), _mul(HERMITE_BASE_H[2,4], M1)))
    c = _add(_add(_mul(HERMITE_BASE_H[3,1], P0), _mul(HERMITE_BASE_H[3,2], P1)),
             _add(_mul(HERMITE_BASE_H[3,3], M0), _mul(HERMITE_BASE_H[3,4], M1)))
    d = _add(_add(_mul(HERMITE_BASE_H[4,1], P0), _mul(HERMITE_BASE_H[4,2], P1)),
             _add(_mul(HERMITE_BASE_H[4,3], M0), _mul(HERMITE_BASE_H[4,4], M1)))
    return (a, b, c, d)
end

function KBSpline(points::Vector{P3}, times_in::AbstractVector{<:Real}=collect(range(0,1,length=length(points)));
                  tension::Real=0, continuity::Real=0, bias::Real=0)
    n = length(points)
    n ≥ 2 || throw(ArgumentError("Need at least 2 points."))

    times = Float32.(collect(times_in))
    length(times) == n || throw(ArgumentError("points and times must have same length."))

    # validate strictly increasing times
    for i in 2:n
        times[i] > times[i-1] || throw(ArgumentError("times must be strictly increasing (failed at i=$i)."))
    end

    T = Float32(tension); C = Float32(continuity); B = Float32(bias)

    # Precompute per-keyframe tangents (time-derivatives)
    mplus  = Vector{P3}(undef, n)   # outgoing at i
    mminus = Vector{P3}(undef, n)   # incoming at i
    for i in 1:n
        mp, mm = _kb_tangents_at(points, times, i, T, C, B)
        mplus[i]  = mp
        mminus[i] = mm
    end

    # Precompute per-segment cubic coefficients in u∈[0,1]
    coeffs = Vector{NTuple{4,P3}}(undef, n-1)
    for i in 1:(n-1)
        t0 = times[i]
        t1 = times[i+1]
        Δt = t1 - t0

        # Map derivative wrt time to derivative wrt u: d/du = Δt * d/dt
        M0 = _mul(Δt, mplus[i])       # outgoing at Pi
        M1 = _mul(Δt, mminus[i+1])    # incoming at P_{i+1}

        coeffs[i] = _hermite_coeffs(points[i], points[i+1], M0, M1)
    end

    return KBSpline(times, coeffs, T, C, B)
end

# Segment lookup and evaluation
@inline function _segment_index(times::Vector{Float32}, t::Float32)
    n = length(times)
    if t ≤ times[1]
        return 1, 0f0
    elseif t ≥ times[end]
        return n-1, 1f0
    else
        i = searchsortedlast(times, t)          # i in 1..n-1
        # map to u∈[0,1]
        u = (t - times[i]) / (times[i+1] - times[i])
        return i, u
    end
end

function (s::KBSpline)(tquery::Real)
    t = Float32(tquery)
    i, u = _segment_index(s.times, t)
    (a,b,c,d) = s.coeffs[i]
    # Horner: (((a*u)+b)*u+c)*u + d
    return _add(_mul(u, _add(_mul(u, _add(_mul(u, a), b)), c)), d)
end

function (s::KBSpline)(tquery::Real)
    t = Float32(tquery)
    i, u = _segment_index(s.times, t)
    (a,b,c,d) = s.coeffs[i]
    # Horner: (((a*u)+b)*u+c)*u + d
    return _add(_mul(u, _add(_mul(u, _add(_mul(u, a), b)), c)), d)
end

#end # module





using GLMakie
using Random

# assuming your module above is available as:
# using .KBSplines

"""
    make_kb_testcase(; rng=MersenneTwister(1),
                      tmin=0f0, tmax=1f0,
                      sweep=(; T=[0f0, 0.5f0, 1f0], C=[-1f0, 0f0, 1f0], B=[-1f0, 0f0, 1f0]))

Creates 5 random Point3f control points, sorted by x, and 5 corresponding strictly
increasing timepoints. Returns `(points, times, splines)` where `splines` is a
vector of `(label::String, spline::KBSplines.KBSpline)` pairs for visual inspection.
"""
function make_kb_testcase(; rng=MersenneTwister(1),
                          tmin::Float32=0f0, tmax::Float32=1f0,
                          sweep=(; T=Float32.(-1:0.1:1), C=Float32.(-1:0.1:1), B=Float32.(-1:0.1:1)))
    @assert tmax > tmin

    # 5 random points in [0,1]^3, then sort by x
    pts = [Point3f(rand(rng), rand(rng), rand(rng)) for _ in 1:5]
    sort!(pts, by = p -> p[1])

    # strictly increasing timepoints (in [tmin,tmax]) based on x-order,
    # with small jitter but enforced monotonicity
    base = range(tmin, tmax; length=5) .|> Float32
    jitter = 0.05f0 * (tmax - tmin)
    ts = Float32[base[i] + (i==1 || i==5 ? 0f0 : (2rand(rng)-1)*jitter) for i in 1:5]
    sort!(ts)
    # enforce strict increasing (in case jitter caused equality)
    eps = 1f-4 * (tmax - tmin)
    for i in 2:5
        if ts[i] ≤ ts[i-1]
            ts[i] = ts[i-1] + eps
        end
    end

    # Build parameter-sweep splines
    splines = Vector{Tuple{String, Any}}()  # Any to avoid type hassles in REPL setups
    for T in sweep.T, C in sweep.C, B in sweep.B
        s = KBSplines.KBSpline(pts, ts; tension=T, continuity=C, bias=B)
        push!(splines, ("T=$(T), C=$(C), B=$(B)", s))
    end

    return pts, ts, splines
end

"""
    plot_kb_splines(points, times, splines;
                    n=100, tmin=times[1], tmax=times[end],
                    which=1:min(6, length(splines)))

Plots selected splines as line plots over [tmin,tmax] sampled at `n` points,
and overlays the control points as a scatter plot.

- `splines` is expected to be a vector of `(label, spline)` pairs (as returned by `make_kb_testcase`).
- `which` selects which entries of `splines` to draw (to avoid 27 lines at once).
Returns `(fig, ax)`.
"""
function plot_kb_splines(points::Vector{Point3f}, times, splines;
                         n::Int=100,
                         tmin::Float32=Float32(times[1]),
                         tmax::Float32=Float32(times[end]),
                         nsplines = 6,
                         which = rand(eachindex(splines),nsplines))
    fig = Figure()
    ax  = Axis3(fig[1, 1];
        xlabel="x", ylabel="y", zlabel="z",
        title="Kochanek–Bartels spline (visual inspection)"
    )

    ts = range(tmin, tmax; length=n) .|> Float32

    for idx in which
        label, s = splines[idx]
        curve = [s(t) for t in ts]
        lines!(ax, curve; label=label)
    end

    scatter!(ax, points; markersize=12)

    axislegend(ax; position=:rt)
    fig
end

# --- Example usage ---
# pts, ts, spls = make_kb_testcase()
# fig = plot_kb_splines(pts, ts, spls; which=1:6)  # plot first 6 parameter combos
# display(fig)



