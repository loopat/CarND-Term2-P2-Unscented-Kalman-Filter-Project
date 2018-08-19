#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>
#include <fstream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
 
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 3;//30;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.7;//30;
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.

  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
  
    is_initialized_ = false;
    time_us_ = 0.0;
    
    n_x_ = 5;
    
    n_aug_ = 7;
    
    lambda_ = 3 - n_aug_;
    
    //Predict sigma points matrix
    Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);
    
    weights_ = VectorXd(2 * n_aug_ + 1);
    
    nis_radar_total = 0;
    nis_radar_7dot8 = 0;
    nis_lidar_total = 0;;
    nis_lidar_5dot99 = 0;
    
}

UKF::~UKF() {

}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
    if( (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) ||
        (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_)){
        /* Initialization */
        if(!is_initialized_){
            x_.fill(0.0);
            
            P_ <<   0.15, 0, 0, 0, 0,
            0, 0.15, 0, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 0, 1, 0,
            0, 0, 0, 0, 1;
            
            if(meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_){
                
                double rho = meas_package.raw_measurements_(0);
                double phi = meas_package.raw_measurements_(1);
                double px = rho * cos(phi);
                double py = rho * sin(phi);

                x_(0) = px;
                x_(1) = py;
                
            } else if(meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_){
                
                double px = meas_package.raw_measurements_(0);
                double py = meas_package.raw_measurements_(1);
                x_(0) = px;
                x_(1) = py;
            }
            is_initialized_ = true;
            time_us_ = meas_package.timestamp_;
            
            return;
        }
        
        double delta_t = (meas_package.timestamp_ - time_us_)/1000000.0;
        time_us_ = meas_package.timestamp_;
   
        /* Prediction */
        Prediction(delta_t);
        
        /* Update measurement */
        if(meas_package.sensor_type_ == MeasurementPackage::RADAR){
            UpdateRadar(meas_package);
        } else if(meas_package.sensor_type_ == MeasurementPackage::LASER){
            UpdateLidar(meas_package);
        }
    }
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */

    /* Generate sigma points */
//    n_x_ = 5;
//    lambda_ = 3 - n_x_;
//    MatrixXd A = P_.llt().matrixL();
//    //Set sigma point matrix
//    Xsig_pred_.col(0) = x_;
//    for(int i = 0; i < n_x_; i ++){
//        Xsig_pred_.col(i + 1)        = x_ + sqrt(lambda_ + n_x_) * A.col(i);
//        Xsig_pred_.col(i + 1 + n_x_) = x_ - sqrt(lambda_ + n_x_) * A.col(i);
//    }
    
    //std:: cout << "\nPredict step starts" << std::endl;
    
    n_aug_ = 7;
    lambda_ = 3 - n_aug_;
    
    //Augmented vector
    VectorXd x_aug = VectorXd(n_aug_);
    x_aug.fill(0.0);
    x_aug.head(n_x_) = x_;
    
    //Create augmented covariance matrix
    MatrixXd P_aug = MatrixXd(n_aug_,n_aug_);
    P_aug.fill(0.0);
    P_aug.topLeftCorner(n_x_,n_x_) = P_;
    P_aug(n_x_,n_x_) = std_a_ * std_a_;
    P_aug(n_x_ + 1, n_x_ + 1) = std_yawdd_ * std_yawdd_;
    
    //Calculate square root of P
    MatrixXd L = P_aug.llt().matrixL();
    
    MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);
    
    //Set sigma points
    //std::cout << "Set sigma points" << std:: endl;
    Xsig_aug.col(0) = x_aug;
    for(int i = 0; i < n_aug_; i ++){
        Xsig_aug.col(i + 1)          = x_aug + sqrt(lambda_ + n_aug_) * L.col(i);
        Xsig_aug.col(i + 1 + n_aug_) = x_aug - sqrt(lambda_ + n_aug_) * L.col(i);
    }
   
    //Predict sigma points
    std::cout << "Predict augmented sigma points" << std:: endl;
    for(int i = 0; i < 2 * n_aug_ + 1; i++){
        double px = Xsig_aug(0,i);
        double py = Xsig_aug(1,i);
        double v  = Xsig_aug(2,i);
        double yaw  = Xsig_aug(3,i);
        double yawd = Xsig_aug(4,i);
        double nu_a = Xsig_aug(5,i);
        double nu_yawdd = Xsig_aug(6,i);
        
        //Predict state values
        double px_p, py_p;
        
        if(fabs(yawd) > 0.001){
            px_p = px + v/yawd * (sin(yaw + yawd * delta_t) - sin(yaw));
            py_p = py + v/yawd * (cos(yaw) - cos(yaw + yawd*delta_t));
        } else {
            px_p = px + v*delta_t*cos(yaw);
            py_p = py + v*delta_t*sin(yaw);
        }
        
        double v_p = v;
        double yaw_p = yaw + yawd * delta_t;
        double yawd_p = yawd;
        
        //Add noise
        px_p = px_p + 0.5 * nu_a * delta_t * delta_t * cos(yaw);
        py_p = py_p + 0.5 * nu_a * delta_t * delta_t * sin(yaw);
        v_p  = v_p  + delta_t * nu_a;
        yaw_p = yaw_p + 0.5 * delta_t * delta_t * nu_yawdd;
        yawd_p = yawd_p + delta_t * nu_yawdd;
        
        //Write predicted sigma points into right coloumn
        Xsig_pred_(0,i) = px_p;
        Xsig_pred_(1,i) = py_p;
        Xsig_pred_(2,i) = v_p;
        Xsig_pred_(3,i) = yaw_p;
        Xsig_pred_(4,i) = yawd_p;
    }
    //std::cout << "Predict set weights" << std:: endl;
    //Set weights
    double weight_0 = lambda_ /(lambda_ + n_aug_);
    weights_(0) = weight_0;
    for(int i = 1; i < 2*n_aug_+1; i++){
        double weight = 0.5/(n_aug_ + lambda_);
        weights_(i) = weight;
    }
    
    //Predict state mean
    //std::cout << "Predict state mean" << std:: endl;
    x_ = Xsig_pred_ * weights_;

    //std::cout << "Predict state covariance matrix" << std:: endl;
    //Predict state covariance matrix
    P_.fill(0.0);
    for(int i = 0; i < 2*n_aug_+1; i++){
        VectorXd diffx = Xsig_pred_.col(i) - x_;
        //while (diffx(3)> M_PI) diffx(3)-=2.*M_PI;
        //while (diffx(3)<-M_PI) diffx(3)+=2.*M_PI;

        P_ += diffx * diffx.transpose() * weights_(i);
        //std::cout << "i= " << i << std::endl;
        //std::cout << diffx(3) << std::endl;
    }
    //std::cout << "Predication Finish" <<std::endl;
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
    //For lidar, n_z is 2.
    int n_z = 2;
    nis_lidar_total += 1;
    
    VectorXd z = meas_package.raw_measurements_;
    
    //Create matrix for sigma points in measurement space
    MatrixXd Zsig = MatrixXd(n_z, 2*n_aug_ + 1);

    // transform sigma points into measurement space
    Zsig.fill(0.0);
    for(int i = 0; i < 2*n_aug_ +1; i++){
        double px = Xsig_pred_(0,i);
        double py = Xsig_pred_(1,i);
        
        Zsig(0,i) = px;
        Zsig(1,i) = py;
    }
    
    //calculate mean predicted measurement
    VectorXd z_pred = VectorXd(n_z);
    z_pred = Zsig * weights_;

    //Create measurement covariance matrix
    MatrixXd S = MatrixXd(n_z, n_z);
    
    //calculate innovation covariance matrix S
    S.fill(0.0);
    
    for(int i = 0; i < (2 * n_aug_ + 1); i++){
        VectorXd diffz = Zsig.col(i) - z_pred;
        S += weights_(i) * diffz * diffz.transpose();
    }
    
    S(0,0) += std_laspx_ * std_laspx_;
    S(1,1) += std_laspy_ * std_laspy_;
    
    //Create matrix for cross correlation Tc
    MatrixXd Tc = MatrixXd(n_x_, n_z);
    
    //Calculate cross correlation matrix
    Tc.fill(0.0);
    for(int i = 0; i < 2*n_aug_ + 1; i++){
        
        VectorXd diffx = Xsig_pred_.col(i) - x_;
     
        VectorXd diffz = Zsig.col(i) - z_pred;
        Tc = Tc + weights_(i) * diffx * diffz.transpose();
    }

    //Calculate Kalman gain K
    MatrixXd K = Tc * S.inverse();

    //Update State mean and covariance matrix
    VectorXd diffz = z - z_pred;
   
    x_ = x_ + K * diffz;
    P_ = P_ - K * S * K.transpose();
    
    //std::cout << "update Lidar" << std::endl;
    
    double nis_laser = diffz.transpose() * S.inverse() * diffz;
    
    //std:: cout << "NIS Lidar is:" << std::endl;
    //std:: cout << nis_laser << std:: endl;
    
    if(nis_laser < 5.991)
    {
        nis_lidar_5dot99 += 1;
    }
    
    std::cout << "percent below 5.99 for lidar is: "<< (nis_lidar_5dot99 * 1.0/nis_lidar_total) << std::endl;
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
    //For radar, n_z is 3.
    int n_z = 3;
    nis_radar_total += 1;
    
    //The measurement
    VectorXd z = meas_package.raw_measurements_;
    
    //Create matrix for sigma points in measurement space
    MatrixXd Zsig = MatrixXd(n_z, 2*n_aug_ + 1);
    
    //Transform sigma points into measurement space
    Zsig.fill(0.0);
    for(int i = 0; i < 2*n_aug_ +1; i++){
        double px = Xsig_pred_(0,i);
        double py = Xsig_pred_(1,i);
        double v  = Xsig_pred_(2,i);
        double yaw = Xsig_pred_(3,i);
        
        double rho = sqrt(px * px + py * py);
        double phi = atan2(py,px);
        double rho_dot = ( px* v * cos(yaw) + py*v * sin(yaw)) / rho;
      
        Zsig(0, i) = rho;
        Zsig(1, i) = phi;
        Zsig(2, i) = rho_dot;
    }

    //calculate mean predicted measurement
    VectorXd z_pred = VectorXd(n_z);
    z_pred = Zsig * weights_;
    //while (z_pred(1)> M_PI) z_pred(1)-=2.*M_PI;
    //while (z_pred(1)<-M_PI) z_pred(1)+=2.*M_PI;

    //Create measurement covariance matrix
    MatrixXd S = MatrixXd(n_z, n_z);
    
    //calculate innovation covariance matrix S
    S.fill(0.0);
    
    for(int i = 0; i < (2 * n_aug_ + 1); i++){
        VectorXd diffz = Zsig.col(i) - z_pred;
        //angle normalization
        //while (diffz(1)> M_PI) diffz(1)-=2.*M_PI;
        //while (diffz(1)<-M_PI) diffz(1)+=2.*M_PI;
        S += weights_(i) * diffz * diffz.transpose();
    }
    
    S(0,0) += std_radr_* std_radr_;
    S(1,1) += std_radphi_ * std_radphi_;
    S(2,2) += std_radrd_ * std_radrd_;

    //Create matrix for cross correlation Tc
    MatrixXd Tc = MatrixXd(n_x_, n_z);
    //Calculate cross correlation matrix
    Tc.fill(0.0);
    for(int i = 0; i < 2*n_aug_ + 1; i++){
        
        VectorXd diffx = Xsig_pred_.col(i) - x_;
        //while(diffx(3) > M_PI) diffx(3) -= 2. * M_PI;
        //while(diffx(3) < M_PI) diffx(3) += 2. * M_PI;
        
        VectorXd diffz = Zsig.col(i) - z_pred;
        while(diffz(1) > M_PI) diffz(1) -= 2. * M_PI;
        while(diffz(1) < M_PI) diffz(1) += 2. * M_PI;
        
        Tc = Tc + weights_(i) * diffx * diffz.transpose();
    }

    //Calculate Kalman gain K
    MatrixXd K = Tc * S.inverse();

    //Update State mean and covariance matrix
    VectorXd diffz = z - z_pred;
    
    x_ = x_ + K * diffz;
    P_ = P_ - K * S * K.transpose();
    
    //std::cout << "Update Radar" << std::endl;
    
    //Calculate NIS
    double nis_radar = diffz.transpose() * S.inverse() * diffz;
    //std::cout << "NIS radar is :"<<std::endl;
    //std::cout << nis_radar<< std::endl;
    
    if(nis_radar < 7.8)
    {
        nis_radar_7dot8 += 1 ;
    }
    
    std::cout << "percent below 7.8 for radar is: "<< (nis_radar_7dot8 * 1.0/nis_radar_total) << std::endl;

}
