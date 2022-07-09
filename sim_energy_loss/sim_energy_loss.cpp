/*
    This program simulates muon decay in the atmosphere and records success rates from different angles (from zenith) of the telescope.
    Author: Jui-Teng Hsu (Roy)
    Summer 2022
*/

#include <iostream>
#include <cmath>
#include <fstream>
#include <random>
using namespace std;

#define c 29979245800 //unit: cm/s
#define m_0 105.6583755 //unit: MeV/c^2
#define m_e 0.51099895 //unit: MeV/c^2
#define tau 2.1969811e-6 //unit: s
#define pi (atan(1)*4)
#define coef 0.1535 //MeVcm^2/g
#define scope_area 1.824146925e2 //cm^2
#define psi (9.7/180 * pi)
#define L_v 10.0e5 //cm
#define dt 1.0e-8

double E_k;
double batch_prob, batch_prob_unadjust;
double factor;

//energy loss porton

void energy_adjust(double E_temp, double& Gamma, double& Beta, double& v){
    Gamma = E_temp / m_0 + 1; //deriving Gamma from kinetic energy
    Beta = sqrt(1-(1/pow(Gamma, 2))); //deriving Beta from Gamma
    v = Beta * c;
}

double delta_correc(double Beta, double Gamma){
    double C_0 = -10.6, X_0 = 1.742, X_1 = 4.28, m = 3.4, a_bethe = 0.1091;
    double X = log10(Beta * Gamma);
    if(X < X_0) return 0.0;
    else if(X < X_1) return 4.6052 * X + C_0 + a_bethe * pow(X_1-X_0, m);
    else return 4.6052 * X + C_0;
}

double shell_correc(double Beta, double Gamma){
    double I = 85.7e-6;
    double eta = Beta * Gamma;
    return (0.422377 * pow(eta, -2.0) + 0.0304043 * pow(eta, -4.0) - 0.00038106 * pow(eta, -6.0)) * 1e6 * pow(I, 2.0)
            +(3.850190 * pow(eta, -2.0) - 0.1667989 * pow(eta, -4.0) + 0.00157955 * pow(eta, -6.0)) * 1e9 * pow(I, 3.0);
}

double bethe_bloch(double W_max, double Gamma, double Beta, double& E_temp, double rho){
    double I = 85.7e-6;
    double Z = 14.51888746, A = 29.09833728, z = 1.0;
    double eta = Gamma * Beta;
    double v = Beta * c;
    double dx = v * dt;
    double dE_k = -1.0 * coef * rho * (Z/A) * pow(z/Beta, 2) * (log(2 * m_e * pow(Gamma * Beta, 2) * W_max / pow(I, 2)) - 2 * pow(Beta, 2)
                    -delta_correc(Beta, Gamma) - 2 * shell_correc(Beta, Gamma) / Z) * dx;
    E_temp += dE_k;
    return dx;
}

//geometrical acceptance portion

void geometric_adjust(double& a, double& b, double& L, double theta){
    L = L_v / cos(theta);
    b = L * tan(psi);
    a = (tan(theta + psi) - tan(theta - psi)) * L_v / 2;
}

double rho_calc(double h){//h in meters
    return 1.225e-3 * pow((288.16-0.0065*h)/288.16, 4.2561); //returns in kg/m^3
}

void batch_calc(double theta){
    double h, k = 0.0;
    double a, b, L;
    geometric_adjust(a, b, L, theta);

    h = L * sin(theta);

    double spacing = 10000.0; //cm
    double probability = 0.0, unadjusted = 0.0;
    for(double i = h - a; i < h + a; i+=spacing){
        for(double j = k - b; j < k + b; j+=spacing){
            if(pow((i-h) / a, 2) + pow((j-k) / b, 2)<=1.0){
                double t_total = 0.0;
                double E_temp = E_k;
                double Gamma, Beta, v;
                energy_adjust(E_temp, Gamma, Beta, v);
                double dist = sqrt(pow(i, 2) + pow(j, 2) + pow(L_v, 2));
                double x = dist;
                unadjusted += 1.0e10 * scope_area / (pow(dist,2) * 2 * pi) * exp(-1.0 * ((dist/v)/Gamma) / tau);
                while(x > 0 && E_temp > 0){
                    energy_adjust(E_temp, Gamma, Beta, v);
                    double eta = Gamma * Beta;
                    double W_max = 2.0 * m_e * eta;
                    double height = cos(theta) * x / 100.0;
                    x -= bethe_bloch(W_max, Gamma, Beta, E_temp, rho_calc(height));
                    t_total += (dt / Gamma);
                }
                if(E_temp<0) continue;
                probability += 1.0e10 * scope_area / (pow(dist,2) * 2 * pi) * exp(-1.0 * t_total / tau); //deriving survival probability
            }
        }
    }
    batch_prob = (double) probability / (a * b / 1.0e10);
    batch_prob_unadjust = (double) unadjusted / (a * b / 1.0e10);
    // batch_prob = (double) not_decayed / (a * b / 1.0e10);
    // batch_prob_unadjust = (double) not_decayed_un / (a * b / 1.0e10);
    if(theta == 0.0) factor = batch_prob;
}


int main(){
    ofstream output("output_2.txt");
    cout<<"Kinetic enrgy of muon(GeV): ";
    double temp;
    cin>>temp;
    E_k = temp * 1000.0; //GeV to MeV
    int n = 25; //number of batches to run
    double theta;
    for(int i = 0; i<n; i++){
        theta = (double)i / (n*2) * (pi - 2 * psi);
        batch_calc(theta);
        cout<<i<<" "<<batch_prob<<" "<<batch_prob_unadjust<<"\n";
        output<<theta/pi*180.0<<" "<<batch_prob<<" "<<batch_prob_unadjust<<" "<<" "<<factor * pow(cos(theta),2)<<"\n";
    }
    output.close();
}