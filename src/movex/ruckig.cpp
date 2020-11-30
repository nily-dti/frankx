#include <complex>
#include <iomanip>

#include <movex/otg/ruckig.hpp>


namespace movex {

void Profile::set(double p0, double v0, double a0, std::array<double, 7> j) {
    this->j = j;
    t_sum[0] = t[0];
    a[0] = a0;
    v[0] = v0;
    p[0] = p0;

    for (size_t i = 0; i < 6; i += 1) {
        t_sum[i+1] = t_sum[i] + t[i+1];
    }
    for (size_t i = 0; i < 7; i += 1) {
        a[i+1] = a[i] + t[i] * j[i];
        v[i+1] = v[i] + t[i] * a[i] + 0.5 * std::pow(t[i], 2) * j[i];
        p[i+1] = p[i] + t[i] * v[i] + 0.5 * std::pow(t[i], 2) * a[i] + 1. / 6 * std::pow(t[i], 3) * j[i];
    }
}

void Profile::reset(double p0, double v0, double a0, double base_jerk) {
    std::array<double, 7> new_j;
    for (size_t step = 0; step < 7; step += 1) {
        if (j[step] > 0.0) {
            new_j[step] = base_jerk;
        } else if (j[step] < 0.0) {
            new_j[step] = -base_jerk;
        }
    }
    set(p0, v0, a0, new_j);
}

bool Profile::check(double pf, double vf, double vMax, double aMax) const {
    // Velocity and acceleration limits can be broken in the beginnging if the initial velocity and acceleration are too high
    // std::cout << std::setprecision(12) << "target: " << p[7] << " " << pf << " " << v[7] << " " << vf << std::endl;
    return std::all_of(t.begin(), t.end(), [](double tm){ return tm >= 0; })
        && std::all_of(v.begin() + 3, v.end(), [vMax](double vm){ return std::abs(vm) < std::abs(vMax) + 1e-9; })
        && std::all_of(a.begin() + 2, a.end(), [aMax](double am){ return std::abs(am) < std::abs(aMax) + 1e-9; })
        && std::abs(p[7] - pf) < 5e-7 && std::abs(v[7] - vf) < 5e-8;
}

std::tuple<double, double, double> Profile::integrate(double t, double p0, double v0, double a0, double j) {
    double p_new = p0 + t * v0 + 0.5 * std::pow(t, 2) * a0 + 1. / 6 * std::pow(t, 3) * j;
    double v_new = v0 + t * a0 + 0.5 * std::pow(t, 2) * j;
    double a_new = a0 + t * j;
    return {p_new, v_new, a_new};
}

inline double Power(double v, int e) {
    return std::pow(v, e);
}

inline double Power(double v, double e) {
    return std::pow(v, e);
}

inline double Sqrt(double v) {
    return std::sqrt(v);
}

inline double Abs(double v) {
    return std::abs(v);
}

inline std::complex<double> Complex(double a, double b) {
    return std::complex<double>(a, b);
}

inline std::complex<double> SqrtComplex(double v) {
    return std::sqrt(std::complex<double>(v, 0));
}

inline std::complex<double> SqrtComplex(std::complex<double> v) {
    return std::sqrt(v);
}

inline std::complex<double> PowerComplex(std::complex<double> v, double e) {
    return std::pow(v, e);
}

bool RuckigEquation::time_up_acc0_acc1_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    profile.t[0] = (-a0 + aMax)/jMax;
    profile.t[1] = (Power(a0,2) - 2*Power(aMax,2) - 2*jMax*v0 + 2*jMax*vMax)/(2.*aMax*jMax);
    profile.t[2] = aMax/jMax;
    profile.t[3] = (3*Power(a0,4) - 8*Power(a0,3)*aMax + 24*a0*aMax*jMax*v0 + 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) - 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*(v0 + vf + 2*vMax) - jMax*(Power(v0,2) + Power(vf,2) - 2*Power(vMax,2))))/(24.*aMax*Power(jMax,2)*vMax);
    profile.t[4] = aMax/jMax;
    profile.t[5] = (-(Power(aMax,2)/jMax) - vf + vMax)/aMax;
    profile.t[6] = aMax/jMax;

    // std::cout << profile.t[0] << std::endl;
    // std::cout << profile.t[1] << std::endl;
    // std::cout << profile.t[2] << std::endl;
    // std::cout << profile.t[3] << std::endl;
    // std::cout << profile.t[4] << std::endl;
    // std::cout << profile.t[5] << std::endl;
    // std::cout << profile.t[6] << std::endl << std::endl;

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    profile.t[0] = (-a0 + aMax)/jMax;
    profile.t[1] = (6*Power(a0,2)*aMax*jMax - 18*Power(aMax,3)*jMax - 12*aMax*Power(jMax,2)*v0 + Sqrt(6)*Sqrt(Power(aMax,2)*Power(jMax,2)*(3*Power(a0,4) - 8*Power(a0,3)*aMax + 24*a0*aMax*jMax*v0 + 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 6*(Power(aMax,4) + 4*aMax*Power(jMax,2)*(-p0 + pf) - 2*Power(aMax,2)*jMax*(v0 + vf) + 2*Power(jMax,2)*(Power(v0,2) + Power(vf,2))))))/(12.*Power(aMax,2)*Power(jMax,2));
    profile.t[2] = aMax/jMax;
    profile.t[3] = 0;
    profile.t[4] = aMax/jMax;
    profile.t[5] = (-18*Power(aMax,3)*jMax - 12*aMax*Power(jMax,2)*vf + Sqrt(6)*Sqrt(Power(aMax,2)*Power(jMax,2)*(3*Power(a0,4) - 8*Power(a0,3)*aMax + 24*a0*aMax*jMax*v0 + 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 6*(Power(aMax,4) + 4*aMax*Power(jMax,2)*(-p0 + pf) - 2*Power(aMax,2)*jMax*(v0 + vf) + 2*Power(jMax,2)*(Power(v0,2) + Power(vf,2))))))/(12.*Power(aMax,2)*Power(jMax,2));
    profile.t[6] = aMax/jMax;

    // std::cout << profile.t[0] << std::endl;
    // std::cout << profile.t[1] << std::endl;
    // std::cout << profile.t[2] << std::endl;
    // std::cout << profile.t[3] << std::endl;
    // std::cout << profile.t[4] << std::endl;
    // std::cout << profile.t[5] << std::endl;
    // std::cout << profile.t[6] << std::endl << std::endl;

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_acc0(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    profile.t[0] = (-2*a0*jMax + Sqrt(2)*Sqrt(Power(a0,2) + 2*jMax*(-v0 + vMax))*Abs(jMax))/(2.*Power(jMax,2));
    profile.t[1] = 0;
    profile.t[2] = (Sqrt(Power(a0,2)/2. + jMax*(-v0 + vMax))*Abs(jMax))/Power(jMax,2);
    profile.t[3] = (-2*jMax*(2*Power(a0,3)*aMax - 6*a0*aMax*jMax*v0 + 3*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*(vf + vMax) + jMax*(-Power(vf,2) + Power(vMax,2)))) + 3*Sqrt(2)*aMax*Sqrt(Power(a0,2) + 2*jMax*(-v0 + vMax))*(Power(a0,2) - 2*jMax*(v0 + vMax))*Abs(jMax))/(12.*aMax*Power(jMax,3)*vMax);
    profile.t[4] = aMax/jMax;
    profile.t[5] = (-(Power(aMax,2)/jMax) - vf + vMax)/aMax;
    profile.t[6] = aMax/jMax;

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_acc1(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    profile.t[0] = (-a0 + aMax)/jMax;
    profile.t[1] = (Power(a0,2) - 2*Power(aMax,2) - 2*jMax*v0 + 2*jMax*vMax)/(2.*aMax*jMax);
    profile.t[2] = aMax/jMax;
    profile.t[3] = ((3*Power(a0,4) - 8*Power(a0,3)*aMax + 24*a0*aMax*jMax*v0 + 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) - 12*jMax*(Power(aMax,2)*(v0 + vMax) + jMax*(-Power(v0,2) + Power(vMax,2)) + 2*aMax*(jMax*(p0 - pf) + SqrtComplex(jMax)*SqrtComplex(-vf + vMax)*(vf + vMax))))/(24.*aMax*Power(jMax,2)*vMax)).real();
    profile.t[4] = (SqrtComplex(-vf + vMax)/SqrtComplex(jMax)).real();
    profile.t[5] = 0;
    profile.t[6] = (SqrtComplex(-vf + vMax)/SqrtComplex(jMax)).real();

    // std::cout << profile.t[0] << std::endl;
    // std::cout << profile.t[1] << std::endl;
    // std::cout << profile.t[2] << std::endl;
    // std::cout << profile.t[3] << std::endl;
    // std::cout << profile.t[4] << std::endl;
    // std::cout << profile.t[5] << std::endl;
    // std::cout << profile.t[6] << std::endl << std::endl;

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_acc0_acc1(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    profile.t[0] = ((-2*a0*jMax + Sqrt(2)*SqrtComplex(Power(a0,2) + 2*jMax*(-v0 + vMax))*Abs(jMax))/(2.*Power(jMax,2))).real();
    profile.t[1] = 0;
    profile.t[2] = (SqrtComplex(Power(a0,2)/2. + jMax*(-v0 + vMax))*Abs(jMax)).real()/Power(jMax,2);
    profile.t[3] = ((-4*jMax*(Power(a0,3) + 3*Power(jMax,2)*(p0 - pf) - 3*a0*jMax*v0 + 3*jMax*SqrtComplex(jMax)*SqrtComplex(-vf + vMax)*(vf + vMax)) + 3*Sqrt(2)*SqrtComplex(Power(a0,2) + 2*jMax*(-v0 + vMax))*(Power(a0,2) - 2*jMax*(v0 + vMax))*Abs(jMax))/(12.*Power(jMax,3)*vMax)).real();
    profile.t[4] = (SqrtComplex(-vf + vMax)/SqrtComplex(jMax)).real();
    profile.t[5] = 0;
    profile.t[6] = (SqrtComplex(-vf + vMax)/SqrtComplex(jMax)).real();

    // std::cout << profile.t[0] << std::endl;
    // std::cout << profile.t[1] << std::endl;
    // std::cout << profile.t[2] << std::endl;
    // std::cout << profile.t[3] << std::endl;
    // std::cout << profile.t[4] << std::endl;
    // std::cout << profile.t[5] << std::endl;
    // std::cout << profile.t[6] << std::endl << std::endl;

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_acc0_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    const double h1 = 5*Power(a0,2) + 6*a0*aMax + Power(aMax,2) + 2*jMax*v0;
    const double h2 = 2*a0 + aMax;
    const double h3 = 3*Power(a0,4) + 8*Power(a0,3)*aMax + 24*a0*aMax*jMax*v0 + 6*Power(a0,2)*(Power(aMax,2) + 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*(v0 + vf) + jMax*(Power(v0,2) - Power(vf,2)));
    const double h4 = (a0 + aMax)*(Power(a0,2) + a0*aMax + 2*jMax*v0);
    const double h5 = 4*Power(a0,4) + 8*Power(a0,3)*aMax + Power(aMax,4) + 24*aMax*Power(jMax,2)*(p0 - pf) - 24*a0*aMax*jMax*v0 + 4*Power(a0,2)*(Power(aMax,2) - 4*jMax*v0) + Power(aMax,2)*jMax*(-8*v0 + 12*vf) + 4*Power(jMax,2)*(4*Power(v0,2) - 3*Power(vf,2));
    const double h6 = 1728*(2*Power(h1,3) - 6*h1*(h3 + 6*h2*h4) + 9*(Power(h2,2)*h3 + 12*Power(h4,2)))*Power(jMax,6);
    const double h7 = Power(h6 + Sqrt(Power(h6,2) - 11943936*Power(h5,3)*Power(jMax,12)),0.3333333333333333);
    const double h8 = Sqrt((4*Power(2,0.3333333333333333)*h5)/h7 + (Power(2,0.6666666666666666)*h7 + 24*(-2*h1 + 3*Power(h2,2))*Power(jMax,2))/(72.*Power(jMax,4)));

    profile.t[0] = -h2/(2.*jMax) + (-12*h8 + Sqrt((-576*Power(2,0.3333333333333333)*h5)/h7 - (2*Power(2,0.6666666666666666)*h7)/Power(jMax,4) - (96*(h1*(3*h2 + 2*h8*jMax) - 3*(Power(h2,3) + 2*h4 + Power(h2,2)*h8*jMax)))/(h8*Power(jMax,3))))/24.;
    profile.t[1] = 0;
    profile.t[2] = -aMax/(2.*jMax) + (-12*h8 + Sqrt((-576*Power(2,0.3333333333333333)*h5)/h7 - (2*Power(2,0.6666666666666666)*h7)/Power(jMax,4) - (96*(h1*(3*h2 + 2*h8*jMax) - 3*(Power(h2,3) + 2*h4 + Power(h2,2)*h8*jMax)))/(h8*Power(jMax,3))))/24.;
    profile.t[3] = 0;
    profile.t[4] = aMax/jMax;
    profile.t[5] = -(12*Power(a0,2)*aMax + jMax*(12*Power(aMax,2)*h8 + aMax*(-12*Power(h8,2)*jMax + h8*jMax*Sqrt((-576*Power(2,0.3333333333333333)*h5)/h7 - (2*Power(2,0.6666666666666666)*h7)/Power(jMax,4) - (96*(h1*(3*h2 + 2*h8*jMax) - 3*(Power(h2,3) + 2*h4 + Power(h2,2)*h8*jMax)))/(h8*Power(jMax,3))) - 24*v0) + h8*jMax*(h8*jMax*Sqrt((-576*Power(2,0.3333333333333333)*h5)/h7 - (2*Power(2,0.6666666666666666)*h7)/Power(jMax,4) - (96*(h1*(3*h2 + 2*h8*jMax) - 3*(Power(h2,3) + 2*h4 + Power(h2,2)*h8*jMax)))/(h8*Power(jMax,3))) + 24*vf)))/(24.*aMax*h8*Power(jMax,2));
    profile.t[6] = aMax/jMax;

    profile.t[2] = (profile.t[2] + profile.t[4]) / 2.;
    profile.t[4] = profile.t[2];

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_acc1_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    if (std::abs(vf) < 1e-16) {
        // Solution 2
        {
            profile.t[0] = (-a0 + aMax)/jMax;
            profile.t[1] = -(-3*Power(a0,2) + 3*Power(aMax,2) + 6*jMax*v0 + Complex(0,1)*Sqrt(3)*SqrtComplex(-3*Power(a0,4) + 8*Power(a0,3)*aMax - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*v0 - jMax*Power(v0,2))) + aMax*SqrtComplex(9*Power(aMax,2) - Complex(0,6)*Sqrt(3)*SqrtComplex(-3*Power(a0,4) + 8*Power(a0,3)*aMax - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*v0 - jMax*Power(v0,2))))).real()/(6.*aMax*jMax);
            profile.t[2] = aMax/jMax;
            profile.t[3] = 0;
            profile.t[4] = (-3*aMax + SqrtComplex(9*Power(aMax,2) - Complex(0,6)*Sqrt(3)*SqrtComplex(-3*Power(a0,4) + 8*Power(a0,3)*aMax - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*v0 - jMax*Power(v0,2))))).real()/(6.*jMax);
            profile.t[5] = 0;
            profile.t[6] = profile.t[4];

            profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
            if (profile.check(pf, vf, vMax, aMax)) {
                return true;
            }
        }

        // Solution 1
        {
            profile.t[0] = (-a0 + aMax)/jMax;
            profile.t[1] = (3*Power(a0,2) - 3*Power(aMax,2) - 6*jMax*v0 - Complex(0,1)*Sqrt(3)*SqrtComplex(-3*Power(a0,4) + 8*Power(a0,3)*aMax - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*v0 - jMax*Power(v0,2))) + aMax*SqrtComplex(9*Power(aMax,2) - Complex(0,6)*Sqrt(3)*SqrtComplex(-3*Power(a0,4) + 8*Power(a0,3)*aMax - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*v0 - jMax*Power(v0,2))))).real()/(6.*aMax*jMax);
            profile.t[2] = aMax/jMax;
            profile.t[3] = 0;
            profile.t[4] = -(3*aMax + SqrtComplex(9*Power(aMax,2) - Complex(0,6)*Sqrt(3)*SqrtComplex(-3*Power(a0,4) + 8*Power(a0,3)*aMax - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*v0 - jMax*Power(v0,2))))).real()/(6.*jMax);
            profile.t[5] = 0;
            profile.t[6] = profile.t[4];

            profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
            if (profile.check(pf, vf, vMax, aMax)) {
                return true;
            }
        }

        return false;
    }

    const double h1 = Power(aMax,2) + 2*jMax*vf;
    const double h2 = 3*Power(a0,4) - 8*Power(a0,3)*aMax + 24*a0*aMax*jMax*v0 + 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) - 12*jMax*(2*aMax*jMax*(p0 - pf) + Power(aMax,2)*(v0 + vf) + jMax*(-Power(v0,2) + Power(vf,2)));
    const double h3 = Power(jMax,4)*(-3*Power(a0,4) + 8*Power(a0,3)*aMax + Power(aMax,4) + 24*aMax*Power(jMax,2)*(p0 - pf) - 24*a0*aMax*jMax*v0 - 6*Power(a0,2)*(Power(aMax,2) - 2*jMax*v0) + 4*Power(aMax,2)*jMax*(3*v0 - 2*vf) + 4*Power(jMax,2)*(-3*Power(v0,2) + 4*Power(vf,2)));
    const double h4 = 1728*Power(jMax,6)*(-2*Power(h1,3) - 6*h1*(h2 - 12*Power(aMax,2)*jMax*vf) + 9*Power(aMax,2)*(h2 - 48*Power(jMax,2)*Power(vf,2)));
    const double h5 = Power(h4 + Sqrt(-11943936*Power(h3,3) + Power(h4,2)),0.3333333333333333);
    const double h6 = Sqrt((-4*Power(2,0.3333333333333333)*h3)/(h5*Power(jMax,4)) - h5/(36.*Power(2,0.3333333333333333)*Power(jMax,4)) + Power(aMax,2)/Power(jMax,2) - (2*h1)/(3.*Power(jMax,2)));
    const double h7 = Sqrt((288*Power(2,0.3333333333333333)*h3*h6 + h5*(Power(2,0.6666666666666666)*h5*h6 + 48*jMax*(3*Power(aMax,3) - 3*aMax*h1 + 3*Power(aMax,2)*h6*jMax - 2*h1*h6*jMax + 12*aMax*jMax*vf)))/(h5*h6*Power(jMax,4)))/(6.*Sqrt(2));

    profile.t[0] = (-a0 + aMax)/jMax;
    profile.t[1] = -(-Power(a0,2) + Power(aMax,2) + jMax*(h6*h7*jMax + 2*v0) + aMax*(-(h6*jMax) + h7*jMax - (2*vf)/h6))/(2.*aMax*jMax);
    profile.t[2] = aMax/jMax;
    profile.t[3] = 0;
    profile.t[4] = -(aMax + h6*jMax - h7*jMax)/(2.*jMax);
    profile.t[5] = 0;
    profile.t[6] = -(aMax + h6*jMax - h7*jMax)/(2.*jMax);

    profile.t[2] = (profile.t[2] + profile.t[4]) / 2.;
    profile.t[4] = profile.t[2];

    profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
    return profile.check(pf, vf, vMax, aMax);
}

bool RuckigEquation::time_up_none(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    if (std::abs(v0) < 1e-16 && std::abs(a0) < 1e-16 && std::abs(vf) < 1e-16) {
        profile.t[0] = Power((pf - p0) / (2 * jMax),1./3);
        profile.t[1] = 0;
        profile.t[2] = profile.t[0];
        profile.t[3] = 0;
        profile.t[4] = profile.t[0];
        profile.t[5] = 0;
        profile.t[6] = profile.t[0];

        profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
        return profile.check(pf, vf, vMax, aMax);
    }

    if (std::abs(v0) < 1e-16 && std::abs(vf) < 1e-16) {
        // Solution 2
        {
            const double h1 = Power(a0,3) + 3*Power(jMax,2)*(p0 - pf);
            const double h2 = -Power(a0,8) + 192*Power(a0,5)*Power(jMax,2)*(p0 - pf) + 288*Power(a0,2)*Power(jMax,4)*Power(p0 - pf,2);
            const double h3 = Power(a0,2)*jMax*(Power(a0,3) + 3*Power(jMax,2)*(p0 - pf));
            const double h4 = 17*Power(a0,6) + 48*Power(a0,3)*Power(jMax,2)*(p0 - pf) + 72*Power(jMax,4)*Power(p0 - pf,2);
            const double h5 = 3*(-576*Power(a0,2)*Power(h3,2) + 96*Power(a0,4)*h1*h3*jMax + 3*Power(a0,12)*Power(jMax,2) + (12*Power(a0,6) + 16*Power(h1,2))*h4*Power(jMax,2));
            // Be careful of numerical stability of h6
            const double h6 = 648*Power(jMax,4)*(h5 + Sqrt(Power(h5,2) - 3*Power(h2,3)*Power(jMax,4)));
            const double h7 = (h2/Power(h6,1./3) + Power(h6,1./3)/(108.*Power(jMax,4)))/Power(a0,2);
            const double h8 = Sqrt(-9*h7 + (3*Power(a0,6) + 4*Power(h1,2))/(Power(a0,4)*Power(jMax,2)))/3.;
            const double h9 = (8*h1*(-27 + (8*Power(h1,2))/Power(a0,6)))/(27.*Power(jMax,3));
            const double h10 = (-6*h8 + Sqrt(36*h7 - (9*h9)/h8 + (8*(3*Power(a0,6) + 4*Power(h1,2)))/(Power(a0,4)*Power(jMax,2))) + (4*h1)/(Power(a0,2)*jMax))/12.;

            profile.t[0] = (-6*h8 + Sqrt(36*h7 - (9*h9)/h8 + (8*(3*Power(a0,6) + 4*Power(h1,2)))/(Power(a0,4)*Power(jMax,2))) - (8*a0)/jMax + (12*jMax*(p0 - pf))/Power(a0,2))/12.;
            profile.t[1] = 0;
            profile.t[2] = h10;
            profile.t[3] = 0;
            profile.t[4] = (-12*Power(a0,7) + 17*Power(a0,6)*h10*jMax + 12*Power(a0,5)*Power(h10,2)*Power(jMax,2) - 18*Power(a0,4)*Power(jMax,2)*(Power(h10,3)*jMax + 2*p0 - 2*pf) + 48*Power(a0,3)*h10*Power(jMax,3)*(p0 - pf) + 36*Power(a0,2)*Power(h10,2)*Power(jMax,4)*(p0 - pf) + 72*h10*Power(jMax,5)*Power(p0 - pf,2))/(-(Power(a0,6)*jMax) + 48*Power(a0,3)*Power(jMax,3)*(p0 - pf) + 72*Power(jMax,5)*Power(p0 - pf,2));
            profile.t[5] = 0;
            profile.t[6] = profile.t[4];

            profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
            profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
            if (profile.check(pf, vf, vMax, aMax)) {
                return true;
            }
        }
    }

    // std::cout << std::setprecision(12) << "---" << std::endl;

    const double h1 = 2*Power(a0,3) + 3*Power(jMax,2)*(-p0 + pf) - 3*a0*jMax*(v0 - 2*vf);
    const double h2 = Power(a0,2) + 2*jMax*(-v0 + vf);
    const double h3 = Power(a0,5) - 24*Power(a0,2)*Power(jMax,2)*(p0 - pf) + 24*Power(jMax,3)*(-p0 + pf)*v0 + 4*Power(a0,3)*jMax*(v0 + 3*vf) + 12*a0*Power(jMax,2)*(Power(v0,2) + 2*v0*vf - Power(vf,2));
    const double h4 = 3*Power(a0,4) - 24*a0*Power(jMax,2)*(p0 - pf) - 4*Power(jMax,2)*Power(v0 - vf,2) + 4*Power(a0,2)*jMax*(v0 + 5*vf);
    const double h5 = Power(a0,6) - 48*Power(a0,3)*Power(jMax,2)*(p0 - pf) - 144*a0*Power(jMax,3)*(p0 - pf)*v0 + 6*Power(a0,4)*jMax*(v0 + 3*vf) + 36*Power(a0,2)*Power(jMax,2)*(Power(v0,2) + 2*v0*vf - Power(vf,2)) - 72*Power(jMax,3)*(jMax*Power(p0 - pf,2) - (v0 - vf)*Power(v0 + vf,2));
    const double h17 = jMax*(-Power(a0,6) + 48*Power(a0,3)*Power(jMax,2)*(p0 - pf) - 144*a0*Power(jMax,3)*(p0 - pf)*v0 + 6*Power(a0,4)*jMax*(v0 - 3*vf) - 36*Power(a0,2)*Power(jMax,2)*(Power(v0,2) - 2*v0*vf - Power(vf,2)) + 72*Power(jMax,3)*(jMax*Power(p0 - pf,2) + Power(v0 - vf,2)*(v0 + vf)));
    const double h6 = -Power(a0,8) + 192*Power(a0,5)*Power(jMax,2)*(p0 - pf) + 8*Power(a0,6)*jMax*(v0 - 5*vf) + 1152*a0*Power(jMax,4)*(p0 - pf)*v0*(v0 + vf) - 192*Power(a0,3)*Power(jMax,3)*(p0 - pf)*(5*v0 + 2*vf) - 120*Power(a0,4)*Power(jMax,2)*(Power(v0,2) - 2*v0*vf - 3*Power(vf,2)) + 96*Power(a0,2)*Power(jMax,3)*(3*jMax*Power(p0 - pf,2) + 5*Power(v0,3) - 3*Power(v0,2)*vf - 15*v0*Power(vf,2) + Power(vf,3)) - 48*Power(jMax,4)*(12*jMax*Power(p0 - pf,2)*(v0 + vf) + Power(v0 - vf,2)*(11*Power(v0,2) + 26*v0*vf + 11*Power(vf,2)));

    const double h8 = 4*Power(h1,2)/(9.*h2) - h4/3.;
    const double h9 = -2*(2*h1/h2*(h8 - h4/6) + h3)/(3*jMax);

    const double h7 = 3*(36*h2*Power(h3,2) + 16*Power(h1,2)*h5 + 3*h4*(Power(h4,2) - 8*h1*h3 - 4*h2*h5));

    // Important: Numerical stability of h10
    const auto h10_x = h6*Power(h6/h7,2);
    auto h10 = PowerComplex(3 * h7 *(1. - SqrtComplex(1 - 3*h10_x)),1./3);
    // std::cout << h10_x << " " << Power(h6/h7,2) << std::endl;
    if (std::abs(Power(h6/h7,2)) < 1e-11) {
        // std::cout << "h10 old: " << h10 << " " << h10_x << std::endl;
        h10 = PowerComplex(h10_x,1./3)*PowerComplex(9*h7/2,1./3) + PowerComplex(h10_x,4./3)*PowerComplex(9*h7/2,1./3)/4. + 5.*PowerComplex(h10_x,7./3)*PowerComplex(9*h7/2,1./3)/16.;

        // h10 = PowerComplex(3,2./3)*h6/PowerComplex(2.*h7,1./3); // + Power(h6,4)/Power(std::abs(h7),8./3)*PowerComplex(9*h7/2,1./3)/4.;
        // std::cout << "h10 new: " << h10 << " " << h10_x << std::endl;
    }

    // std::cout << h10 << " " << h10_x << std::endl;
    // const auto h11 = h6/(6.*h10) + h10/18.;
    const auto h11 = std::complex<double>(h10.real()/18 + (h10.real() * h6)/(6*std::norm(h10)), h10.imag()/18 - (h10.imag() * h6)/(6*std::norm(h10)));

    // std::cout << h6 << " " << h7 << " " << h8 << " " << h9 << " " << h10 << " " << h11 << std::endl;

    auto h11_h2 = h11/h2;
    auto h8_h2 = h8/h2;
    auto h12 = SqrtComplex(h11_h2 + h8_h2)/jMax;

    if (std::abs(h11+h8) < 1e-3) {
        // std::cout << std::setprecision(16) << "h11: " << h11 << std::endl;
        // std::cout << "h8: " << h8 << std::endl;
        // std::cout << "old h11+h8: " << h11+h8 << std::endl;

        // std::cout << "old h12: " << h12 << std::endl;

        h12 = SqrtComplex(h6/(6.*h10*h2) + Power(2*h1,2)/(Power(3*h2, 2)) + (h10 - 6*h4)/(18.*h2))/jMax;
        // std::cout << "new h12: " << h12 << std::endl;
    }

    // auto h12_x_1 = (h11/h2).imag();
    // auto h12_alt = std::complex<double>(
    //     std::sqrt(h8/h2) + std::pow(h12_x_1, 2)/(8*Power(h8/h2,3./2)),
    //     h12_x_1/(2*std::sqrt(h8/h2)) - (std::pow(h12_x_1,3))/(16 * Power(h8/h2,5./2))
    // )/(jMax);

    // std::cout << "h12: " << h12 << std::endl;

    auto h9_h12_real = (h12.real() * h9)/(std::norm(h12)*h2);
    auto h9_h12_imag = (-h12.imag() * h9)/(std::norm(h12)*h2);

    auto h12_a = SqrtComplex(-h11_h2 + 2*h8_h2 + h9_h12_real + std::complex<double>(0, h9_h12_imag))/jMax;
    auto h12_b = SqrtComplex(-h11_h2 + 2*h8_h2 - h9_h12_real - std::complex<double>(0, h9_h12_imag))/jMax;

    // std::cout << h11 + h8 << std::endl;
    // std::cout << -h11 + 2*h8 - h9/h12 << std::endl;
    // std::cout << h12_b << std::endl;

    auto h13_c = (h12 - h12_a)/2. - h1/(3.*h2*jMax);
    auto h14_c = (h12 + h12_a)/2. - h1/(3.*h2*jMax);
    auto h15_c = (-h12 + h12_b)/2. - h1/(3.*h2*jMax);
    auto h16_c = (-h12 - h12_b)/2. - h1/(3.*h2*jMax);

    // std::cout << "h13-16: " << std::endl;
    // std::cout << h13_c << std::endl;
    // std::cout << h14_c << std::endl;
    // std::cout << h15_c << std::endl;
    // std::cout << h16_c << std::endl;


    // Solution 3
    if (h13_c.real() > 0.0 && std::abs(h13_c.imag()) < 1e-8) {
        const double h13 = std::abs(h13_c);

        profile.t[0] = h13;
        profile.t[1] = 0;
        profile.t[2] = (-4*Power(a0,3) + 3*jMax*(h2*(h12 - h12_a) + 2*jMax*(p0 - pf)) + 6*a0*(h2 + jMax*(v0 - 2*vf))).real()/(6.*h2*jMax);
        profile.t[3] = 0;
        profile.t[4] = -((Power(a0,7) + 13*Power(a0,6)*h13*jMax + 72*Power(jMax,4)*(-(h13*(jMax*Power(p0 - pf,2) - Power(v0 - vf,3))) + Power(h13,2)*jMax*(p0 - pf)*(v0 - vf) + 2*(p0 - pf)*v0*(v0 - vf) + Power(h13,3)*jMax*Power(v0 - vf,2)) + 6*Power(a0,5)*jMax*(7*Power(h13,2)*jMax + v0 + 3*vf) - 12*Power(a0,3)*Power(jMax,2)*(10*h13*jMax*(p0 - pf) - Power(v0,2) + Power(h13,2)*jMax*(13*v0 - 16*vf) - 2*v0*vf + 3*Power(vf,2)) + 6*Power(a0,4)*Power(jMax,2)*(3*Power(h13,3)*jMax - 8*p0 + 8*pf + h13*(v0 + 19*vf)) - 36*Power(a0,2)*Power(jMax,3)*(Power(h13,2)*jMax*(p0 - pf) + 2*(-p0 + pf)*v0 + 2*Power(h13,3)*jMax*(v0 - vf) + h13*(3*Power(v0,2) + 2*v0*vf - 3*Power(vf,2))) - 72*a0*Power(jMax,3)*(Power(v0,3) + Power(v0,2)*vf - 3*v0*Power(vf,2) + Power(vf,3) + jMax*(Power(p0,2) + Power(pf,2) + h13*pf*(4*v0 - 2*vf) - 2*p0*(pf + 2*h13*v0 - h13*vf) + Power(h13,2)*(-2*Power(v0,2) + 5*v0*vf - 3*Power(vf,2)))))/h17);
        profile.t[5] = 0;
        profile.t[6] = profile.t[4];

        // Set average as only sum of t2 and t4 needs to be >0
        profile.t[2] = (profile.t[2] + profile.t[4]) / 2.;
        profile.t[4] = profile.t[2];

        profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
        if (profile.check(pf, vf, vMax, aMax)) {
            return true;
        }
    }

    // Solution 4
    if (h14_c.real() > 0.0 && std::abs(h14_c.imag()) < 1e-8) {
        const double h14 = std::abs(h14_c);

        profile.t[0] = h14;
        profile.t[1] = 0;
        profile.t[2] = (-4*Power(a0,3) + 3*jMax*(h2*(h12 + h12_a) + 2*jMax*(p0 - pf)) + 6*a0*(h2 + jMax*(v0 - 2*vf))).real()/(6.*h2*jMax);
        profile.t[3] = 0;
        profile.t[4] = -((Power(a0,7) + 13*Power(a0,6)*h14*jMax + 72*Power(jMax,4)*(-(h14*(jMax*Power(p0 - pf,2) - Power(v0 - vf,3))) + Power(h14,2)*jMax*(p0 - pf)*(v0 - vf) + 2*(p0 - pf)*v0*(v0 - vf) + Power(h14,3)*jMax*Power(v0 - vf,2)) + 6*Power(a0,5)*jMax*(7*Power(h14,2)*jMax + v0 + 3*vf) - 12*Power(a0,3)*Power(jMax,2)*(10*h14*jMax*(p0 - pf) - Power(v0,2) + Power(h14,2)*jMax*(13*v0 - 16*vf) - 2*v0*vf + 3*Power(vf,2)) + 6*Power(a0,4)*Power(jMax,2)*(3*Power(h14,3)*jMax - 8*p0 + 8*pf + h14*(v0 + 19*vf)) - 36*Power(a0,2)*Power(jMax,3)*(Power(h14,2)*jMax*(p0 - pf) + 2*(-p0 + pf)*v0 + 2*Power(h14,3)*jMax*(v0 - vf) + h14*(3*Power(v0,2) + 2*v0*vf - 3*Power(vf,2))) - 72*a0*Power(jMax,3)*(Power(v0,3) + Power(v0,2)*vf - 3*v0*Power(vf,2) + Power(vf,3) + jMax*(Power(p0,2) + Power(pf,2) + h14*pf*(4*v0 - 2*vf) - 2*p0*(pf + 2*h14*v0 - h14*vf) + Power(h14,2)*(-2*Power(v0,2) + 5*v0*vf - 3*Power(vf,2)))))/h17);
        profile.t[5] = 0;
        profile.t[6] = profile.t[4];

        // std::cout << std::setprecision(9) << profile.t[0] << std::endl;
        // std::cout << profile.t[1] << std::endl;
        // std::cout << profile.t[2] << std::endl;
        // std::cout << profile.t[3] << std::endl;
        // std::cout << profile.t[4] << std::endl;
        // std::cout << profile.t[5] << std::endl;
        // std::cout << profile.t[6] << std::endl << std::endl;

        profile.t[2] = (profile.t[2] + profile.t[4]) / 2.;
        profile.t[4] = profile.t[2];

        profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
        if (profile.check(pf, vf, vMax, aMax)) {
            return true;
        }
    }

    // Solution 2
    if (h15_c.real() > 0.0 && std::abs(h15_c.imag()) < 1e-8) {
        const double h15 = h15_c.real();

        profile.t[0] = h15;
        profile.t[1] = 0;
        profile.t[2] = (-4*Power(a0,3) + 3*jMax*(h2*(-h12 + h12_b) + 2*jMax*(p0 - pf)) + 6*a0*(h2 + jMax*(v0 - 2*vf))).real()/(6.*h2*jMax);
        profile.t[3] = 0;
        profile.t[4] = -((Power(a0,7) + 13*Power(a0,6)*h15*jMax + 72*Power(jMax,4)*(-(h15*(jMax*Power(p0 - pf,2) - Power(v0 - vf,3))) + Power(h15,2)*jMax*(p0 - pf)*(v0 - vf) + 2*(p0 - pf)*v0*(v0 - vf) + Power(h15,3)*jMax*Power(v0 - vf,2)) + 6*Power(a0,5)*jMax*(7*Power(h15,2)*jMax + v0 + 3*vf) - 12*Power(a0,3)*Power(jMax,2)*(10*h15*jMax*(p0 - pf) - Power(v0,2) + Power(h15,2)*jMax*(13*v0 - 16*vf) - 2*v0*vf + 3*Power(vf,2)) + 6*Power(a0,4)*Power(jMax,2)*(3*Power(h15,3)*jMax - 8*p0 + 8*pf + h15*(v0 + 19*vf)) - 36*Power(a0,2)*Power(jMax,3)*(Power(h15,2)*jMax*(p0 - pf) + 2*(-p0 + pf)*v0 + 2*Power(h15,3)*jMax*(v0 - vf) + h15*(3*Power(v0,2) + 2*v0*vf - 3*Power(vf,2))) - 72*a0*Power(jMax,3)*(Power(v0,3) + Power(v0,2)*vf - 3*v0*Power(vf,2) + Power(vf,3) + jMax*(Power(p0,2) + Power(pf,2) + h15*pf*(4*v0 - 2*vf) - 2*p0*(pf + 2*h15*v0 - h15*vf) + Power(h15,2)*(-2*Power(v0,2) + 5*v0*vf - 3*Power(vf,2)))))/h17);
        profile.t[5] = 0;
        profile.t[6] = profile.t[4];

        // std::cout << profile.t[0] << std::endl;
        // std::cout << profile.t[1] << std::endl;
        // std::cout << profile.t[2] << std::endl;
        // std::cout << profile.t[3] << std::endl;
        // std::cout << profile.t[4] << std::endl;
        // std::cout << profile.t[5] << std::endl;
        // std::cout << profile.t[6] << std::endl << std::endl;

        profile.t[2] = (profile.t[2] + profile.t[4]) / 2.;
        profile.t[4] = profile.t[2];

        profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
        if (profile.check(pf, vf, vMax, aMax)) {
            return true;
        }
    }

    // Solution 1
    if (h16_c.real() > 0.0 && std::abs(h16_c.imag()) < 1e-8) {
        const double h16 = h16_c.real();

        profile.t[0] = h16;
        profile.t[1] = 0;
        profile.t[2] = (-4*Power(a0,3) + 3*jMax*(h2*(-h12 - h12_b) + 2*jMax*(p0 - pf)) + 6*a0*(h2 + jMax*(v0 - 2*vf))).real()/(6.*h2*jMax);
        profile.t[3] = 0;
        profile.t[4] = -((Power(a0,7) + 13*Power(a0,6)*h16*jMax + 72*Power(jMax,4)*(-(h16*(jMax*Power(p0 - pf,2) - Power(v0 - vf,3))) + Power(h16,2)*jMax*(p0 - pf)*(v0 - vf) + 2*(p0 - pf)*v0*(v0 - vf) + Power(h16,3)*jMax*Power(v0 - vf,2)) + 6*Power(a0,5)*jMax*(7*Power(h16,2)*jMax + v0 + 3*vf) - 12*Power(a0,3)*Power(jMax,2)*(10*h16*jMax*(p0 - pf) - Power(v0,2) + Power(h16,2)*jMax*(13*v0 - 16*vf) - 2*v0*vf + 3*Power(vf,2)) + 6*Power(a0,4)*Power(jMax,2)*(3*Power(h16,3)*jMax - 8*p0 + 8*pf + h16*(v0 + 19*vf)) - 36*Power(a0,2)*Power(jMax,3)*(Power(h16,2)*jMax*(p0 - pf) + 2*(-p0 + pf)*v0 + 2*Power(h16,3)*jMax*(v0 - vf) + h16*(3*Power(v0,2) + 2*v0*vf - 3*Power(vf,2))) - 72*a0*Power(jMax,3)*(Power(v0,3) + Power(v0,2)*vf - 3*v0*Power(vf,2) + Power(vf,3) + jMax*(Power(p0,2) + Power(pf,2) + h16*pf*(4*v0 - 2*vf) - 2*p0*(pf + 2*h16*v0 - h16*vf) + Power(h16,2)*(-2*Power(v0,2) + 5*v0*vf - 3*Power(vf,2)))))/h17);
        profile.t[5] = 0;
        profile.t[6] = profile.t[4];

        // std::cout << profile.t[0] << std::endl;
        // std::cout << profile.t[1] << std::endl;
        // std::cout << profile.t[2] << std::endl;
        // std::cout << profile.t[3] << std::endl;
        // std::cout << profile.t[4] << std::endl;
        // std::cout << profile.t[5] << std::endl;
        // std::cout << profile.t[6] << std::endl << std::endl;

        profile.t[2] = (profile.t[2] + profile.t[4]) / 2.;
        profile.t[4] = profile.t[2];

        profile.set(p0, v0, a0, {jMax, 0, -jMax, 0, -jMax, 0, jMax});
        if (profile.check(pf, vf, vMax, aMax)) {
            return true;
        }
    }
    return false;
}

bool RuckigEquation::time_down_acc0_acc1_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_acc0_acc1_vel(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_vel(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_acc0(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_acc0(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_acc1(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_acc1(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_acc0_acc1(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_acc0_acc1(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_acc0_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_acc0_vel(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_acc1_vel(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_acc1_vel(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

bool RuckigEquation::time_down_none(Profile& profile, double p0, double v0, double a0, double pf, double vf, double vMax, double aMax, double jMax) {
    return time_up_none(profile, p0, v0, a0, pf, vf, -vMax, -aMax, -jMax);
}

double RuckigEquation::jerk_to_reach_target_with_times(const std::array<double, 7>& t, double p0, double v0, double a0, double pf) {
    const double t1 {t[0]}, t2 {t[1]}, t3 {t[2]}, t4 {t[3]}, t5 {t[4]}, t6 {t[5]}, t7 {t[6]};
    return -((-6*p0 + 6*pf - 3*(t1 + t2 + t3 + t4 + t5 + t6 + t7)*(a0*(t1 + t2 + t3 + t4 + t5 + t6 + t7) + 2*v0))/(-Power(t1,3) + Power(t3,3) + Power(t5,3) + 3*Power(t5,2)*t6 + 3*t5*Power(t6,2) + 3*Power(t5,2)*t7 + 6*t5*t6*t7 + 3*t5*Power(t7,2) - Power(t7,3) + 3*Power(t3,2)*(t4 + t5 + t6 + t7) + 3*t3*Power(t4 + t5 + t6 + t7,2) - 3*Power(t1,2)*(t2 + t3 + t4 + t5 + t6 + t7) - 3*t1*Power(t2 + t3 + t4 + t5 + t6 + t7,2)));
}

} // namespace movex
