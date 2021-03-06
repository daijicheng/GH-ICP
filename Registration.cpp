#include <cstring>
#include <cstdio>
#include <fstream>
#include <Eigen/dense>
#include "dataIo.h"
#include "stdlib.h"
#include "KM.h"
#include "Registration.h"
#include <string.h>
#include <iostream> 
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <liblas/liblas.hpp>
#include <liblas/version.hpp>
#include <liblas/point.hpp>
#include "dataIo.h"
#include "keypointDetection.h"
#include "StereoBinaryFeature.h"
#include "fpfh.h"

#include <pcl\filters\extract_indices.h>
#include <pcl/visualization/common/common.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include "utility.h"

#include <boost/filesystem.hpp>
#include <boost/function.hpp>

#include <liblas/liblas.hpp>
#include <liblas/version.hpp>
#include <liblas/point.hpp>

#include <fstream>
#include <string.h>


using namespace KMSpace;
using namespace std;
using namespace Reg;
using namespace Eigen;
using namespace boost::filesystem;

void Registration::calED()
{
	//double minED = 10000.0, maxED =0.0;
	for (size_t i = 0; i <KP.kps_num; ++i)
	{
		for (size_t j = 0; j < KP.kpt_num; ++j)
		{
			//for TLS
			EF.ED[i][j] = EF.k*sqrt(pow(KP.kpSXYZ(i,0) - KP.kpTXYZ(j,0), 2) + pow(KP.kpSXYZ(i,1) - KP.kpTXYZ(j,1), 2) + pow(KP.kpSXYZ(i,2) - KP.kpTXYZ(j,2), 2));
			//for bunny
			//EF.ED[i][j] = 200.0*sqrt(pow(KP.kpSXYZ(i, 0) - KP.kpTXYZ(j, 0), 2) + pow(KP.kpSXYZ(i, 1) - KP.kpTXYZ(j, 1), 2) + pow(KP.kpSXYZ(i, 2) - KP.kpTXYZ(j, 2), 2));
			//minED = min(minED, EF.ED[i][j]);
			//maxED = max(maxED, EF.ED[i][j]);
			
		}
	}
	//It's ok if there's no normalization or we can try another way of normalization
	//normalization ED
	/*for (size_t i = 0; i < KP.kps_num; ++i)
	{
		for (size_t j = 0; j < KP.kpt_num; ++j)
		{
			EF.ED[i][j] = (EF.ED[i][j] - minED) / (maxED - minED);
		}
	}//ED OK*/
}
void  Registration::calFD_BSC(const doubleVectorSBF &bscS, const doubleVectorSBF &bscT)
{
	/*
	ofstream ofs;
	ostringstream oss;
	oss << " Hamming distance.txt";
	ofs.open(oss.str());

	if (ofs.is_open())
	{
	for (size_t i = 0; i < keyPointIndicesS->indices.size(); ++i)
	{
	for (size_t j = 0; j < keyPointIndicesT->indices.size(); ++j)
	{
	ofs << sbf.hammingDistance(bscS[0][i], bscT[0][j]) << "  "
	<< sbf.hammingDistance(bscS[0][i], bscT[1][j]) << "  "
	<< sbf.hammingDistance(bscS[1][i], bscT[0][j]) << "  "
	<< sbf.hammingDistance(bscS[1][i], bscT[1][j]) << endl;
	}
	}
	ofs.close();
	}*/

	
	SBF sbf0;
	//int minFD = 1000, maxFD = 0;
	for (size_t i = 0; i < KP.kps_num; ++i)
	{
		for (size_t j = 0; j<KP.kpt_num; ++j)
		{
			EF.FD[i][j] = min(min(sbf0.hammingDistance(bscS[0][i], bscT[0][j]), sbf0.hammingDistance(bscS[0][i], bscT[1][j])),
				min(sbf0.hammingDistance(bscS[1][i], bscT[0][j]), sbf0.hammingDistance(bscS[1][i], bscT[1][j])));

			//if (EF.FD[i][j] < minFD) minFD = EF.FD[i][j];
			//if (EF.FD[i][j] > maxFD) maxFD = EF.FD[i][j];
		}
	}
	cout << "FD calculation completed." << endl;
	
	//----------------------------------------

	//normalization FD
	/*for (size_t i = 0; i < keyPointIndicesS->indices.size(); ++i){
	for (size_t j = 0; j < keyPointIndicesT->indices.size(); ++j){
	FD[i][j] = (FD[i][j] - minFD) / (maxFD - minFD);
	}
	}
	//FD OK*/
}

void  Registration::calFD_FPFH(const fpfhFeaturePtr &fpfhS, const fpfhFeaturePtr &fpfhT)
{
	FPFHfeature fpfhcal;
	for (size_t i = 0; i < KP.kps_num; ++i)
	{
		for (size_t j = 0; j <KP.kpt_num; ++j)
		{
			EF.FD[i][j] = fpfhcal.compute_fpfh_distance(fpfhS->points[i].histogram, fpfhT->points[j].histogram);  //(0-1)
		}
	}
	cout << "FD calculation completed." << endl;
}

void  Registration::calCD_NF(){
	double CDsum = 0;
	double CDmean;
	for (size_t i = 0; i < KP.kps_num; i++){
		for (size_t j = 0; j < KP.kpt_num; j++){
			EF.CD[i][j] = EF.ED[i][j];
			CDsum += EF.CD[i][j];
		}
	}//CD OK
	CDmean = CDsum / KP.kpt_num / KP.kps_num;
	// Assign the value of penalty
	if (iteration_number > 1){
		EF.penalty = RMS*EF.para1_penalty*EF.k;
	}
	else{
		EF.penalty = CDmean/EF.penalty_initial;
	}	
	cout << "CDmean: " << CDmean << "  penalty:" << EF.penalty << endl;
}

void  Registration::calCD_BSC()
{
	double WFD = exp(-1.0*iteration_number / EF.m);
	//double WFD = 0.0;
	double WED = 1.0 - WFD;
	double CDsum = 0;
	double CDmean, CDstd;
	double CDstdsum = 0;
	//CDmax = 0;
	for (size_t i = 0; i < KP.kps_num; i++){
		for (size_t j = 0; j <KP.kpt_num; j++){

				EF.CD[i][j] = WED*EF.ED[i][j] + WFD*EF.FD[i][j];
				CDsum+=EF.CD[i][j];			
		}
	}//CD OK
	//cout << KP.kps_num << "pts, " << KP.kpt_num << "pts" << endl;
	CDmean = CDsum / KP.kpt_num / KP.kps_num;
	for (size_t i = 0; i < KP.kps_num; ++i){
		for (size_t j = 0; j < KP.kpt_num; ++j){
			CDstdsum += pow(EF.CD[i][j] - CDmean, 2);
		}
	}
	CDstd = sqrt(CDstdsum / KP.kpt_num / KP.kps_num);
	//EF.penalty = (CDmean - 2*CDstd)/2;//penalty OK
	//EF.penalty = CDmean / 3;
	//if (EF.para1_penalty>=EF.penalty_threshod) EF.para1_penalty -= EF.para2_penalty;//1.25
	
	// Assign the value of penalty

	if (iteration_number > 1){
			EF.penalty = RMS*EF.para1_penalty*EF.k*WED + (FDM + EF.para2_penalty*FDstd)*WFD;
	}
	else{
		EF.penalty = (CDmean - EF.penalty_initial * CDstd);
	}
	//EF.penalty = CDmean / 3.0;
	//EF.penalty = (CDmean - EF.para1_penalty * CDstd);  //this one
	//EF.penalty = 10000;
	//EF.penalty = min(CDmean / EF.para2_penalty, CDmean - EF.para1_penalty * CDstd);
	cout << "weight FD:  " << WFD << "  weight ED:  " << WED << endl;
	cout << "CDmean: " << CDmean <<" CDstd: "<< CDstd<<"  penalty:"<<EF.penalty << endl;
}

void  Registration::calCD_FPFH()  // In this case, FD is the metric of similarity so that it ranges from 0 to 1
{
	
	double CDsum = 0;
	double CDmean = 0;
	double CDstd = 0;
	double CDstdsum = 0;

	for (size_t i = 0; i < KP.kps_num; i++){
		for (size_t j = 0; j < KP.kpt_num; j++){
	        //Important calculation here
			EF.CD[i][j] =1.0* EF.ED[i][j] / pow(EF.FD[i][j],1.0/(iteration_number+1));  //CD=ED/(FS^(1/k)) (here FS is the similarity and k is the iteration number count from 1)
			
			CDsum += EF.CD[i][j];
			//cout << CDsum<< endl;
		}
	}


	CDmean = CDsum / KP.kps_num / KP.kpt_num;
	/*for (size_t i = 0; i < KP.kps_num; ++i){
		for (size_t j = 0; j < KP.kpt_num; ++j){
			//CDstdsum += pow(EF.CD[i][j] - CDmean, 2);
		}
	}*/
	//CDstd = sqrt(CDstdsum / KP.kps_num / KP.kpt_num);

	// Assign the value of penalty
	
	if (iteration_number > 1){
		                                                            //Use feature: GH-ICP
		EF.penalty = RMS*EF.para1_penalty*EF.k*EF.para2_penalty;    //penalty= RMS*scale*ped/(1/pfd)=RMS*scale*ped*pfd     Notice that 1/pfd must be 0-1 so that pfd must be greater than 1.

	}
	else{                                                             //The first iteration
		EF.penalty = (CDmean / EF.penalty_initial );                  //penalty= CDmean/p1
	}

	//cout << "weight FD:  " << WFD << "  weight ED:  " << WED << endl;
	cout << "CDmean: " << CDmean << "  penalty:" << EF.penalty << endl;
}

void  Registration::findcorrespondenceKM()
{
	
	//F2 modified method

    //graph weight
	int size = max(KP.kps_num, KP.kpt_num);
	std::vector<vector<double>> graphweight(size, vector<double>(size));
	//const double INF = 1000;
	for (size_t i = 0; i <size; ++i)
	{
		for (size_t j = 0; j <size; ++j)
		{
			graphweight[i][j] = -EF.penalty;
		}
	}
	for (size_t i = 0; i <KP.kps_num; ++i)
	{
		for (size_t j = 0; j <KP.kpt_num; ++j)
		{
			if (EF.CD[i][j] < EF.penalty) graphweight[i][j] = -EF.CD[i][j];
		}
	}
	cout << "Weight OK" << endl;
	//F1 initial method 
	/*
	//graph weight
	int size = KP.kps_num + KP.kpt_num - EF.min_cor;
	std::vector<vector<double>> graphweight(size,vector<double>(size));
	//const int INF = 200;
	//for (size_t i = KP.kps_num; i < size; ++i)
	//{
	//	for (size_t j = KP.kpt_num; j < size; ++j)
	//	{
	//		graphweight[i][j] = -INF;
	//	}
	//}
	for (size_t i = 0; i <KP.kps_num; ++i)
	{
		for (size_t j = 0; j <KP.kpt_num; ++j)
		{
			graphweight[i][j] = -EF.CD[i][j];
		}
	}
	for (size_t i = 0; i < KP.kps_num; ++i)
	{
		for (size_t j =KP.kpt_num; j < size; ++j)
		{
			graphweight[i][j] =-EF.penalty;
		}
	}
	for (size_t i = KP.kps_num; i < size; ++i)
	{
		for (size_t j = 0; j <KP.kpt_num; ++j)
		{
			graphweight[i][j] = -EF.penalty;
		}
	}
	for (size_t i = KP.kps_num; i < size; ++i)
	{
		for (size_t j = KP.kpt_num; j < size; ++j)
		{
			graphweight[i][j] = -EF.penalty;
		}
	}//Weight OK
	
	cout << "Weight OK" << endl;
	*/

	/*-------KM--------*/
	Graph graph;
	graph.GTable = graphweight;
	graph.sp = KP.kps_num;
	graph.tp = KP.kpt_num;
	graph.n = size;
	graph.lx.resize(graph.n);
	graph.ly.resize(graph.n);
	graph.match.resize(graph.n);
	graph.slack.resize(graph.n);
	graph.visx.resize(graph.n);
	graph.visy.resize(graph.n);
	//cout << "Graoh OK" << endl;
	Km kmsolver(graph,EF.KM_eps,EF.penalty);
	kmsolver.kmsolve();
	
	
	vector<int> SP;
	vector<int> TP;
	//used for test
	vector<int> SPout;
	vector<int> TPout;

	int cor_number=kmsolver.output(SP,TP,SPout,TPout);
	double minenergy = kmsolver.energy();
	cout << "Energy:  " << minenergy << endl;
	energy.push_back(minenergy);
	cor.push_back(cor_number);

	pre.push_back(kmsolver.precision);
	rec.push_back(kmsolver.recall);

	Spoint.resize(cor_number, 3);
	Tpoint.resize(cor_number, 3);
	for (int i = 0; i < cor_number; i++){
		Spoint.row(i) = KP.kpSXYZ.row(SP[i]);
		Tpoint.row(i) = KP.kpTXYZ.row(TP[i]);
	}//update Spoint,Tpoint
	for (int i = 0; i < cor_number; i++){
		matchlist[SP[i]][iteration_number] = TP[i];
	}
	for (int i = 0; i < SPout.size(); i++){
		matchlist[SPout[i]][iteration_number] = -1;
	}
	
	
	
	// Output final matched key points coordinate  (used for test)
	/*
	if (iteration_number > 1){
		ofstream ofs;
		ofs.open("final matched S.txt");

		if (ofs.is_open())
		{
			for (size_t i = 0; i < cor_number; ++i)
			{
				ofs << setiosflags(ios::fixed) << setprecision(3) << Spoint(i, 0) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << Spoint(i, 1) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << Spoint(i, 2) << endl;
			}
			ofs.close();
		}

		ofs.open("final matched T.txt");

		if (ofs.is_open())
		{
			for (size_t i = 0; i < cor_number; ++i)
			{
				ofs << setiosflags(ios::fixed) << setprecision(3) << Tpoint(i, 0) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << Tpoint(i, 1) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << Tpoint(i, 2) << endl;
			}
			ofs.close();
		}

		ofs.open("final unmatched S.txt");

		if (ofs.is_open())
		{
			for (size_t i = 0; i < SPout.size(); ++i)
			{
				ofs << setiosflags(ios::fixed) << setprecision(3) << KP.kpSXYZ(SPout[i], 0) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << KP.kpSXYZ(SPout[i], 1) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << KP.kpSXYZ(SPout[i], 2) << endl;
			}
			ofs.close();
		}

		ofs.open("final unmatched T.txt");

		if (ofs.is_open())
		{
			for (size_t i = 0; i < TPout.size(); ++i)
			{
				ofs << setiosflags(ios::fixed) << setprecision(3) << KP.kpTXYZ(TPout[i], 0) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << KP.kpTXYZ(TPout[i], 1) << "  "
					<< setiosflags(ios::fixed) << setprecision(3) << KP.kpTXYZ(TPout[i], 2) << endl;
			}
			ofs.close();
		}
	}*/

	//cal PCFD 
	//if (iteration_number == 0)  PCFD=calCloudFeatureDistance(cor_number);

	//Output energy fraction
	
	double energy_ED = 0;
	double energy_FD = 0;
	double energy_penalty = 0;
	for (int i = 0; i < cor_number; i++){
		energy_ED += EF.ED[SP[i]][TP[i]];
		energy_FD += EF.FD[SP[i]][TP[i]];
	}
	
	/*double WED, WFD;
	if (iteration_number == 0) WED = 0;
	else  WED = 1;
	WFD = 1 - WED;*/
	//double WFD = 0.0;

	double WFD = exp(-1.0*iteration_number / EF.m);
	double WED = 1.0 - WFD;
	energy_ED *= WED;
	energy_FD *= WFD;
	//energy_penalty = (size - cor_number)*EF.penalty;
	//cout << "energy fraction: ED  " << energy_ED << "  FD  " << energy_FD << "  penalty  " << energy_penalty << endl;
	//cout << "energy fraction:  ED: " << energy_ED << "  FD:  " << energy_FD  << endl;
	cout << cor_number << " pairs matched " << endl;
	
	//cal RMSE and FDM
	double RMSE=0;
	double FDcul = 0;
	FDM = 0;
	FDstd = 0;
	for (size_t i = 0; i < cor_number; ++i)
	{
			RMSE += pow(Spoint(i, 0) - Tpoint(i, 0), 2) + pow(Spoint(i, 1) - Tpoint(i, 1), 2) + pow(Spoint(i, 2) - Tpoint(i, 2), 2);
			FDM += EF.FD[SP[i]][TP[i]];
	}
	FDM /= cor_number;
	for (size_t i = 0; i < cor_number; ++i)
	{
		FDcul += pow((EF.FD[SP[i]][TP[i]]-FDM),2);
	}
	FDstd = sqrt(FDcul / cor_number);
	RMSE /= cor_number;
	RMSE = sqrt(RMSE);
	rmse.push_back(RMSE);
	cout << "correspondence RMSE  " << RMSE << "   mean FD  " << FDM << "   std FD  "<< FDstd<< endl;
	double deltaRMS = RMS - RMSE;
	double para_RMS = deltaRMS / RMSE;
	
	/*
	//use this to jump out from the local optimum
	if (RMSE > 0.5 && RMSE < 5 && para_RMS < 0.04)
	{
		iteration_k = 1;   //because we do k++ after this iteration
		cout << "----------------jump out----------------" << endl;
	}*/
	RMS = RMSE;
	
	//converge condition
	//if (deltaRMS < 0 && RMSE < 0.5) converge = 1;
	//if (deltaRMS < 0 ) converge = 1;

	//
	//
	/*
	ofstream ofs3;
	ostringstream oss3;
	oss3 << iteration_number << " keypoint transform.txt";
	ofs3.open(oss3.str());
	if (ofs3.is_open())
	{
		for (size_t i = 0; i < cor_number; ++i)
		{
			ofs3 << setiosflags(ios::fixed) << setprecision(3) << Tpoint(i, 0) << "  "
				<< setiosflags(ios::fixed) << setprecision(3) << Tpoint(i, 1) << "  "
				<< setiosflags(ios::fixed) << setprecision(3) << Tpoint(i, 2) << endl;
		}
		ofs3.close();
	}
	*/
}
void  Registration::findcorrespondenceNNR(){
	//Use the closest point in target cloud as the correspondence for each point in source cloud (H-ICP) 
	//H-ICP is more efficient than GH-ICP with o(n2)
	
	//Reciprocal optimal strategy 
	//Problem: Too few correspondence found
	//cout << "Finding correspondence ... ..." << endl;
	
	vector<int> SV;
	vector<int> TV;
	vector<int> SP;
	vector<int> TP;
	double MAXVALIUE = 9e20;
	double mincd = MAXVALIUE;
	int minindex = 0;
	int cor_number = 0;
	for (size_t i = 0; i < KP.kps_num; i++){
		for (size_t j = 0; j < KP.kpt_num; j++){
			if (EF.CD[i][j] < mincd)
			{
				mincd = EF.CD[i][j];
			    minindex = j;
			}	
		}
		SV.push_back(minindex);
		minindex = 0;
		mincd = MAXVALIUE;
	}


	for (size_t i = 0; i < KP.kpt_num; i++){
		for (size_t j = 0; j < KP.kps_num; j++){
			if (EF.CD[j][i] < mincd)
			{
				mincd = EF.CD[j][i];
				minindex = j;
			}
		}
		TV.push_back(minindex);
		minindex = 0;
		mincd = MAXVALIUE;
	}
	

	for (size_t i = 0; i < KP.kps_num; i++){
		if (TV[SV[i]]==i)
		{
			SP.push_back(i);
			TP.push_back(SV[i]);
			cor_number++;
			
			cout << SP[cor_number - 1] << " - " << TP[cor_number - 1] << endl;
		}
	}
	
	//先Resize啊，没分配内存会报错的
	Spoint.resize(cor_number, 3);
	Tpoint.resize(cor_number, 3);

	for (size_t i = 0; i < cor_number; i++){
		Spoint.row(i) = KP.kpSXYZ.row(SP[i]);
		Tpoint.row(i) = KP.kpTXYZ.row(TP[i]);
	}  //update Spoint,Tpoint


	cout << cor_number << " pairs matched. " << endl;

	//cal RMSE and FDM
	double RMSE = 0;
	double FDcul = 0;
	FDM = 0;
	FDstd = 0;
	for (size_t i = 0; i < cor_number; ++i)
	{
		RMSE += pow(Spoint(i, 0) - Tpoint(i, 0), 2) + pow(Spoint(i, 1) - Tpoint(i, 1), 2) + pow(Spoint(i, 2) - Tpoint(i, 2), 2);
		FDM += EF.FD[SP[i]][TP[i]];
	}
	FDM /= cor_number;
	for (size_t i = 0; i < cor_number; ++i)
	{
		FDcul += pow((EF.FD[SP[i]][TP[i]] - FDM), 2);
	}
	FDstd = sqrt(FDcul / cor_number);
	RMSE /= cor_number;
	RMSE = sqrt(RMSE);
	rmse.push_back(RMSE);
	cout << "correspondence RMSE  " << RMSE << "   mean FD  " << FDM << "   std FD  " << FDstd << endl;
	RMS = RMSE;

}

void  Registration::findcorrespondenceNN(){
	//Use the closest point in target cloud as the correspondence for each point in source cloud (H-ICP) 
	//H-ICP is more efficient than GH-ICP with o(n2)

	//Find Closest Point (less than penalty) in Target Cloud

	//cout << "Finding correspondence ... ..." << endl;

	vector<int> SP;
	vector<int> TP;
	double MAXVALIUE = 9e20;
	double mincd = MAXVALIUE;
	int minindex = 0;
	int cor_number = 0;
	for (size_t i = 0; i < KP.kps_num; i++){
		for (size_t j = 0; j < KP.kpt_num; j++){
			if (EF.CD[i][j] < mincd)
			{
				mincd = EF.CD[i][j];
				minindex = j;
			}
		}
		if (mincd < EF.penalty)
		{
			SP.push_back(i);
			TP.push_back(minindex);
			cor_number++;
		}
		minindex = 0;
		mincd = MAXVALIUE;
	}


	//先Resize啊，没分配内存会报错的
	Spoint.resize(cor_number, 3);
	Tpoint.resize(cor_number, 3);

	for (size_t i = 0; i < cor_number; i++){
		Spoint.row(i) = KP.kpSXYZ.row(SP[i]);
		Tpoint.row(i) = KP.kpTXYZ.row(TP[i]);
	}  //update Spoint,Tpoint


	cout << cor_number << " pairs matched. " << endl;

	//cal RMSE and FDM
	double RMSE = 0;
	double FDcul = 0;
	FDM = 0;
	FDstd = 0;
	for (size_t i = 0; i < cor_number; ++i)
	{
		RMSE += pow(Spoint(i, 0) - Tpoint(i, 0), 2) + pow(Spoint(i, 1) - Tpoint(i, 1), 2) + pow(Spoint(i, 2) - Tpoint(i, 2), 2);
		FDM += EF.FD[SP[i]][TP[i]];
	}
	FDM /= cor_number;
	for (size_t i = 0; i < cor_number; ++i)
	{
		FDcul += pow((EF.FD[SP[i]][TP[i]] - FDM), 2);
	}
	FDstd = sqrt(FDcul / cor_number);
	RMSE /= cor_number;
	RMSE = sqrt(RMSE);
	rmse.push_back(RMSE);
	cout << "correspondence RMSE  " << RMSE << "   mean FD  " << FDM << "   std FD  " << FDstd << endl;
	RMS = RMSE;

}

void Registration::adjustweight(double estimated_IoU)
{
	if (estimated_IoU / IoU > 1.2)
	{
		EF.para1_penalty += 0.1;
		EF.para2_penalty += 0.1;
	}
	else if (IoU/ estimated_IoU > 1.2)
	{
		EF.para1_penalty -= 0.1;
		EF.para2_penalty -= 0.1;
	}
	else{ ; }
	cout << "IoU:  "<<IoU<<"  Penalty_ED: " << EF.para1_penalty << "  Penalty_FD: " << EF.para2_penalty << endl;
}


void  Registration::transformestimation(Eigen::Matrix4d &Rt)
{
	
	
	//gravitization
	int cor_number=Spoint.rows();
	if (cor_number < EF.min_cor) converge = 1;
	//Estimate overlapping rate. Attention! Integer->Double
	IoU = 1.0*cor_number / (KP.kps_num + KP.kpt_num-cor_number); 
	/*for (int i = 0; i < cor_number; i++)
	{
		Spoint.row(i) -= Spoint.colwise().sum()/cor_number;
		Tpoint.row(i) -= Tpoint.colwise().sum()/cor_number;
    }
	//SVD for Rt
	Eigen::Matrix3d  H;
	H = Tpoint.transpose()*Spoint;
	JacobiSVD<MatrixXd> svd(H, ComputeThinU | ComputeThinV);
	Eigen::Matrix3d R;
	Eigen::Vector3d t;
	R = svd.matrixV()*(svd.matrixU().transpose());
	
	
	int i = 1;
	while(R.determinant()<0) { // get rid of the risk of reflection transform
		//cout << "the reflection case" << endl;
		Eigen::MatrixXd M(cor_number, cor_number);
		M = Eigen::MatrixXd::Identity(cor_number, cor_number);
		M(cor_number - i, cor_number - i) = -1;
		R = svd.matrixV()*M;
		R = R*(svd.matrixU().transpose());
		i++;	
	}
	cout << "the reflection case: " << i << endl;
	
	t = (Spoint.colwise().sum() / cor_number).transpose() - R*(Tpoint.colwise().sum() / cor_number).transpose();
	
	Rt=Eigen::Matrix4d::Zero();
	Rt.block(0, 0, 3, 3) = R;
	Rt.block(0, 3, 3, 1) = t;
	Rt(3, 3) = 1;
	cout << "this Rt" << endl;
	cout << Rt << endl;
	//
	*/
	
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_in(new pcl::PointCloud<pcl::PointXYZ>());
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_out(new pcl::PointCloud<pcl::PointXYZ>());
	cloud_in->points.resize(cor_number);
	cloud_out->points.resize(cor_number);
	for (int i = 0; i<cor_number; i++)
	{
		cloud_in->points[i].x = Spoint(i, 0);
		cloud_in->points[i].y = Spoint(i, 1);
		cloud_in->points[i].z = Spoint(i, 2);
	}

	for (int i = 0; i<cor_number; i++)
	{
		cloud_out->points[i].x = Tpoint(i, 0);
		cloud_out->points[i].y = Tpoint(i, 1);
		cloud_out->points[i].z = Tpoint(i, 2);
	}

	pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ> TSVD;
	pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ>::Matrix4 Rt0;
	TSVD.estimateRigidTransformation(*cloud_in, *cloud_out, Rt0);
	Eigen::Matrix3d R;
	Eigen::Vector3d t;
	//Rt0 = Rt0.inverse();
	R = Rt0.block(0, 0, 3, 3).cast<double>();
	t = Rt0.block(0, 3, 3, 1).cast<double>();

	Rt = Rt0.cast<double>();
	cout << "this Rt" << endl;
	cout << Rt << endl;
	

	double dx = t(0);
	double dy = t(1);
	double dz = t(2);
	double ax = atan2(R(2, 1), R(2, 2));
	double ay = atan2(-R(2, 0), sqrt(R(2, 1)*R(2, 1) + R(2, 2)*R(2, 2)));
	double az = atan2(R(0, 1), R(0, 0));
	const double pi = 3.1415926;
	ax = ax / pi * 180;
	ay = ay / pi * 180;
	az = az / pi * 180;

	cout << "Angle_X: " << ax << "   Angle_Y: " << ay << "   Angle_Z: " << az << endl;
	cout << "Translate_X: " << dx << "   Translate_Y: " << dy << "   Translate_Z: " << dz << endl;

	/*if (abs(dx) < 0.05 && abs(dy) < 0.05 && abs(dz) < 0.05 && abs(ax) < 0.02 && abs(ay) < 0.02 && abs(az) < 0.02) //converge condition
	{
		converge = 1;
	}*/
	

	if (abs(dx) < converge_t && abs(dy) < converge_t && abs(dz) <converge_t && abs(ax) < converge_r && abs(ay) < converge_r && abs(az) < converge_r ) //converge condition
	{
		converge = 1;
	}

	//Update
	double RMSEafter = 0;
	for (int i = 0; i < KP.kps_num; i++) //Finally I find the source of bug. What a fool I am !!!!!  
	{
		KP.kpSXYZ.row(i) = (R*(KP.kpSXYZ.row(i)).transpose() + t).transpose();
	}
	for (int i = 0; i < Spoint.rows(); i++)
	{
		Spoint.row(i) = (R*(Spoint.row(i)).transpose() + t).transpose();
	}
	for (size_t i = 0; i < Spoint.rows(); ++i)
	{
		RMSEafter += pow(Spoint(i, 0) - Tpoint(i, 0), 2) + pow(Spoint(i, 1) - Tpoint(i, 1), 2) + pow(Spoint(i, 2) - Tpoint(i, 2), 2);	
	}
	RMSEafter /= Tpoint.rows();
	RMSEafter = sqrt(RMSEafter);
	cout << "RMSE after transformation: " << RMSEafter <<endl;
	
	rmseafter.push_back(RMSEafter);
	//cout << KP.kpTXYZ << endl;
	if (converge == 1){
		if (RMSEafter < nonmax)cout << "Registration Succeed." << endl;
		else cout << "Registration Failed." << endl;
	}
}

/*void  Registration::update(Eigen::Matrix4Xd &TKP, Eigen::Matrix4Xd &TFP, Eigen::Matrix4d &Rt)
{
	TKP = Rt*TKP;
	TFP = Rt*TFP;
	KP.kpTXYZ = (TKP.block(0,0,TKP.cols(),3)).transpose();

}*/

void Registration::displayPCrb(const pcXYZIPtr &cloudT, Eigen::Matrix4Xd &cloudS, Eigen::Matrix4Xd &KpT, Eigen::Matrix4Xd &kpS)
{
	//红+蓝
	Eigen::Matrix<double, 4, Dynamic> CloudS;
    CloudS.resize(4, cloudS.cols());
	CloudS = Rt_tillnow*cloudS;
	
	Eigen::Matrix<double, 4, Dynamic> KpS;
	KpS.resize(4, kpS.cols());
	KpS = Rt_tillnow*kpS;

	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->setBackgroundColor(255, 255, 255);
	char t[256];
	string s;
	int n = 0;

	pcXYZRGBPtr pointcloudS(new pcXYZRGB());
	pcXYZRGBPtr pointcloudT(new pcXYZRGB());

	for (size_t i = 0; i < cloudT->points.size(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = cloudT->points[i].x;
		pt.y = cloudT->points[i].y;
		pt.z = cloudT->points[i].z;
		pt.r = 255;
		pt.g = 215;
		pt.b = 0;
		pointcloudT->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudT, "pointcloudT");


	for (size_t i = 0; i < cloudS.cols(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = CloudS(0, i) ;
		pt.y = CloudS(1, i) ;
		pt.z = CloudS(2, i);
		pt.r = 233;
		pt.g = 233;
		pt.b = 216;
		pointcloudS->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudS, "pointcloudS");

	for (size_t i = 0; i < KpS.cols(); ++i)
	{
		pcl::PointXYZ pt;
		pt.x = KpS(0, i);
		pt.y = KpS(1, i);
		pt.z = KpS(2, i);
		sprintf(t, "%d", n);
		s = t;
		viewer->addSphere(pt, 0.001, 1.0, 0.0, 0.0, s); //small scale
		//viewer->addSphere(pt, 0.8, 1.0, 0.0, 0.0, s); //large scale
		n++;
	
	}

	for (size_t i = 0; i < KpT.cols(); ++i)
	{
		pcl::PointXYZ pt;
		pt.x = KpT(0, i);
		pt.y = KpT(1, i);
		pt.z = KpT(2, i);
		sprintf(t, "%d", n);
		s = t;
		viewer->addSphere(pt, 0.001, 0.0, 0.0, 1.0, s); //small scale
		//viewer->addSphere(pt, 0.8, 0.0, 0.0, 1.0, s);  //large scale
		n++;

	}

	cout << "Click X(close) to continue..."<<endl;
	while (!viewer->wasStopped())
	{
		viewer->spinOnce(100);
		boost::this_thread::sleep(boost::posix_time::microseconds(100000));
	}
}

void Registration::displayPCvb(const pcXYZIPtr &cloudT, Eigen::Matrix4Xd &cloudS, Eigen::Matrix4Xd &KpT, Eigen::Matrix4Xd &kpS)
{
	//紫+蓝
	Eigen::Matrix<double, 4, Dynamic> CloudS;
	CloudS.resize(4, cloudS.cols());
	CloudS = Rt_tillnow*cloudS;

	Eigen::Matrix<double, 4, Dynamic> KpS;
	KpS.resize(4, kpS.cols());
	KpS = Rt_tillnow*kpS;

	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("Registered Result"));
	viewer->setBackgroundColor(255, 255, 255);
	char t[256];
	string s;
	int n = 0;

	pcXYZRGBPtr pointcloudS(new pcXYZRGB());
	pcXYZRGBPtr pointcloudT(new pcXYZRGB());

	for (size_t i = 0; i < cloudT->points.size(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = cloudT->points[i].x;
		pt.y = cloudT->points[i].y;
		pt.z = cloudT->points[i].z;
		pt.r = 255;
		pt.g = 215;
		pt.b = 0;
		pointcloudT->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudT, "pointcloudT");


	for (size_t i = 0; i < cloudS.cols(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = CloudS(0, i);
		pt.y = CloudS(1, i);
		pt.z = CloudS(2, i);
		pt.r = 233;
		pt.g = 233;
		pt.b = 216;
		pointcloudS->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudS, "pointcloudS");

	for (size_t i = 0; i < KpS.cols(); ++i)
	{
		pcl::PointXYZ pt;
		pt.x = KpS(0, i);
		pt.y = KpS(1, i);
		pt.z = KpS(2, i);
		sprintf(t, "%d", n);
		s = t;
		viewer->addSphere(pt, 0.001, 1.0, 0.0, 1.0, s); //small scale
		//viewer->addSphere(pt, 0.8, 1.0, 0.0, 1.0, s); //large scale
		n++;

	}

	for (size_t i = 0; i < KpT.cols(); ++i)
	{
		pcl::PointXYZ pt;
		pt.x = KpT(0, i);
		pt.y = KpT(1, i);
		pt.z = KpT(2, i);
		sprintf(t, "%d", n);
		s = t;
		viewer->addSphere(pt, 0.001, 0.0, 0.0, 1.0, s); //small scale
		//viewer->addSphere(pt, 0.8, 0.0, 0.0, 1.0, s);  //large scale
		n++;

	}

	cout << "Click X(close) to continue..." << endl;
	while (!viewer->wasStopped())
	{
		viewer->spinOnce(100);
		boost::this_thread::sleep(boost::posix_time::microseconds(100000));
	}
}

void Registration::displayPCyb(const pcXYZIPtr &cloudT, Eigen::Matrix4Xd &cloudS, Eigen::Matrix4Xd &KpT, Eigen::Matrix4Xd &kpS)
{
	//黄+蓝
	Eigen::Matrix<double, 4, Dynamic> CloudS;
	CloudS.resize(4, cloudS.cols());
	CloudS = Rt_tillnow*cloudS;

	Eigen::Matrix<double, 4, Dynamic> KpS;
	KpS.resize(4, kpS.cols());
	KpS = Rt_tillnow*kpS;

	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->setBackgroundColor(255, 255, 255);
	char t[256];
	string s;
	int n = 0;

	pcXYZRGBPtr pointcloudS(new pcXYZRGB());
	pcXYZRGBPtr pointcloudT(new pcXYZRGB());

	for (size_t i = 0; i < cloudT->points.size(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = cloudT->points[i].x;
		pt.y = cloudT->points[i].y;
		pt.z = cloudT->points[i].z;
		pt.r = 255;
		pt.g = 215;
		pt.b = 0;
		pointcloudT->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudT, "pointcloudT");


	for (size_t i = 0; i < cloudS.cols(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = CloudS(0, i);
		pt.y = CloudS(1, i);
		pt.z = CloudS(2, i);
		pt.r = 233;
		pt.g = 233;
		pt.b = 216;
		pointcloudS->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudS, "pointcloudS");

	for (size_t i = 0; i < KpS.cols(); ++i)
	{
		pcl::PointXYZ pt;
		pt.x = KpS(0, i);
		pt.y = KpS(1, i);
		pt.z = KpS(2, i);
		sprintf(t, "%d", n);
		s = t;
		viewer->addSphere(pt, 0.001, 1.0, 1.0, 0.0, s); //small scale
		//viewer->addSphere(pt, 0.8, 1.0, 1.0, 0.0, s); //large scale
		n++;

	}

	for (size_t i = 0; i < KpT.cols(); ++i)
	{
		pcl::PointXYZ pt;
		pt.x = KpT(0, i);
		pt.y = KpT(1, i);
		pt.z = KpT(2, i);
		sprintf(t, "%d", n);
		s = t;
		viewer->addSphere(pt, 0.001, 0.0, 0.0, 1.0, s); //small scale
		//viewer->addSphere(pt, 0.8, 0.0, 0.0, 1.0, s);  //large scale
		n++;

	}

	cout << "Click X(close) to continue..." << endl;
	while (!viewer->wasStopped())
	{
		viewer->spinOnce(100);
		boost::this_thread::sleep(boost::posix_time::microseconds(100000));
	}
}

void Registration::displayCorrespondence(const pcXYZIPtr &cloudS, Eigen::Matrix4Xd &TP)
{
	Eigen::Matrix<double, 4, Dynamic> TransPC;
	TransPC.resize(4, TP.cols());
	TransPC = Rt_tillnow*TP;

	boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));
	viewer->setBackgroundColor(0, 0, 0);
	char t[256];
	string s;
	int n = 0;

	pcXYZRGBPtr pointcloudS(new pcXYZRGB());
	pcXYZRGBPtr pointcloudT(new pcXYZRGB());

	for (size_t i = 0; i < cloudS->points.size(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = cloudS->points[i].x;
		pt.y = cloudS->points[i].y;
		pt.z = cloudS->points[i].z;
		pt.r = 0;
		pt.g = 255;
		pt.b = 0;
		pointcloudS->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudS,"pointcloudS");


	for (size_t i = 0; i < TP.cols(); ++i)
	{
		pcl::PointXYZRGB pt;
		pt.x = TransPC(0, i) + 50.0f;
		pt.y = TransPC(1, i) + 50.0f;
		pt.z = TransPC(2, i);
		pt.r = 0;
		pt.g = 0;
		pt.b = 255;
		pointcloudT->points.push_back(pt);
	}

	viewer->addPointCloud(pointcloudT, "pointcloudT");

	for (size_t i = 0; i < Tpoint.rows(); ++i)
	{
		if (i % 10 == 0)
		{
			pcl::PointXYZ pt1, pt2;
			pt1.x = Tpoint(i, 0) + 50.0;
			pt1.y = Tpoint(i, 1) + 50.0;
			pt1.z = Tpoint(i, 2);

			sprintf(t, "%d", n);
			s = t;
			viewer->addSphere(pt1, 0.05, 1.0, 0.0, 0.0, s);
			n++;


			pt2.x = Spoint(i, 0);
			pt2.y = Spoint(i, 1);
			pt2.z = Spoint(i, 2);

			sprintf(t, "%d", n);
			s = t;
			viewer->addSphere(pt2, 0.05, 1.0, 0.0, 0.0, s);
			n++;

			sprintf(t, "%d", n);
			s = t;
			viewer->addLine(pt1, pt2, 1.0, 0.0, 0.0, s);
			viewer->setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_LINE_WIDTH, 2, s);
			n++;
		}
		
	}	

	while (!viewer->wasStopped())
	{
		viewer->spinOnce(100);
		boost::this_thread::sleep(boost::posix_time::microseconds(100000));
	}
}

void Registration::save(const pcXYZIPtr &cloudfS, const pcXYZIPtr &cloudS, Eigen::MatrixX3d &kpSXYZ_0, Eigen::MatrixX3d &kpTXYZ_0){
	sP.resize(4, cloudS->size());
	sfP.resize(4, cloudfS->size());
	tkP.resize(4, kpTXYZ_0.rows());
	skP.resize(4, kpSXYZ_0.rows());

	for (size_t i = 0; i < cloudS->size(); ++i)
	{
		sP(0, i) = cloudS->points[i].x;
		sP(1, i) = cloudS->points[i].y;
		sP(2, i) = cloudS->points[i].z;
		sP(3, i) = 1;
	}
	for (size_t i = 0; i < cloudfS->size(); ++i)
	{
		sfP(0, i) = cloudfS->points[i].x;
		sfP(1, i) = cloudfS->points[i].y;
		sfP(2, i) = cloudfS->points[i].z;
		sfP(3, i) = 1;
	}
	for (size_t i = 0; i < kpTXYZ_0.rows(); ++i)
	{
		tkP(0, i) = kpTXYZ_0(i, 0);
		tkP(1, i) = kpTXYZ_0(i, 1);
		tkP(2, i) = kpTXYZ_0(i, 2);
		tkP(3, i) = 1;
	}
	for (size_t i = 0; i < kpSXYZ_0.rows(); ++i)
	{
		skP(0, i) = kpSXYZ_0(i, 0);
		skP(1, i) = kpSXYZ_0(i, 1);
		skP(2, i) = kpSXYZ_0(i, 2);
		skP(3, i) = 1;
	}
}



//pcl::PointCloud<pcl::PointXYZI>::Ptr Registration::output(const pcXYZIPtr &cloud, Eigen::Matrix4d &Rt)
void Registration::output(Eigen::Matrix4Xd &SP)
{
	
	//Eigen::Matrix<double, 3, Dynamic> TP;
	//TP.resize(3, cloud->size());
	
	//TP.col(i) = (Rt.block(0, 0, 3, 3)*(TP.col(i)) + Rt.block(0, 3, 3, 1));
	//cloud->points[i].x = TP(0, i);
	//cloud->points[i].y = TP(1, i);
	//cloud->points[i].z = TP(2, i);
	Eigen::Matrix<double, 4, Dynamic> TransPC;
	TransPC.resize(4, SP.cols());
	TransPC = Rt_tillnow*SP;

	ofstream ofs;
	ostringstream oss;
	oss << iteration_number <<" After transform.txt";
	if (index_output == 0)		ofs.open(oss.str());
	else ofs.open("SC_keypoint_after_transform.txt");

	if (ofs.is_open())
	{
		for (size_t i = 0; i < TransPC.cols(); ++i)
		{
			
			if (converge == 1){
				
				if (i % 2 == 0){ //最终结果再抽稀输出 (实验用，后期去）
					
					ofs << setiosflags(ios::fixed) << setprecision(3) << TransPC(0, i) << "  "
						<< setiosflags(ios::fixed) << setprecision(3) << TransPC(1, i) << "  "
						<< setiosflags(ios::fixed) << setprecision(3) << TransPC(2, i) << endl;
					
				}

			}
			else
			{
				if (i % 10 == 0){ //再抽稀输出
					ofs << setiosflags(ios::fixed) << setprecision(3) << TransPC(0, i) << "  "
						<< setiosflags(ios::fixed) << setprecision(3) << TransPC(1, i) << "  "
						<< setiosflags(ios::fixed) << setprecision(3) << TransPC(2, i) << endl;
				}
			}

		}
		ofs.close();
	}

	cout << "Output finished ... ..." << endl;
	if (index_output == 1){
		ofstream ofs2;
		ofs2.open("Final Rt.txt");
		if (ofs2.is_open())
		{
			ofs2 << Rt_tillnow << endl;
			ofs2.close();
		}
		cout << "Output final RT......." << endl;
	}

	//return cloud;
	//output transformed point cloud
}


void Registration::energyRMSoutput(){
	
	energy.resize(iteration_number);
	rmse.resize(iteration_number);
	cor.resize(iteration_number);

	ofstream ofs;
	
	ofs.open("energy.txt");
	if (ofs.is_open())
	{
		for (size_t i = 0; i < energy.size(); ++i)
		{
			ofs << i + 1 << "\t" << energy[i] << endl;
		}
		ofs.close();
	}

	ofs.open("RMSE.txt");
	if (ofs.is_open())
	{
		for (size_t i = 0; i < rmse.size(); ++i)
		{
			ofs << i+1 <<"\t"<<rmse[i] << endl;
		}
		ofs.close();
	}

	ofs.open("cor number.txt");
	if (ofs.is_open())
	{
		for (size_t i = 0; i < cor.size(); ++i)
		{
			ofs << i + 1 << "\t"<<cor[i] << endl;
		}
		ofs.close();
	}

	ofs.open("RMSE after transformation.txt");
	if (ofs.is_open())
	{
		for (size_t i = 0; i < rmseafter.size(); ++i)
		{
			ofs << i + 1 << "\t" << rmseafter[i] << endl;
		}
		ofs.close();
	}

	ofs.open("P and R.txt");
	if (ofs.is_open())
	{
		ofs << "iteration\tprecision\trecall\n";
		for (size_t i = 0; i < rec.size(); ++i)
		{
			ofs << i + 1 << "\t" << pre[i]<<"\t"<<rec[i] << endl;
		}
		ofs.close();
	}
}


void Registration::calGTmatch(Eigen::Matrix4Xd &SKP, Eigen::Matrix4Xd &TKP0)
{
	Eigen::Matrix<double, 4, Dynamic> TKP;
	TKP.resize(4, TKP0.cols());
	TKP= Rt_gt*TKP0;

	//calculate ground truth corresponence

	double dis;
	
	gtmatchlist.resize(SKP.cols());

	for (int i = 0; i < SKP.cols(); i++)
	{
		double tempdis = 1.0;
		int tempj = -1;
		for (int j = 0; j < TKP.cols(); j++)
		{
			dis = 1.0*sqrt(pow((SKP(0, i) - TKP(0, j)), 2) + pow((SKP(1, i) - TKP(1, j)), 2) + pow((SKP(2, i) - TKP(2, j)), 2));
			if (dis < gt_maxdis && dis < tempdis){
				tempdis = dis;
				tempj = j;
			}
		}
		gtmatchlist[i] = tempj;
	}
	/*for (int i=0; i < SKP.cols(); ++i)
	{
		cout <<i<<"   "<< gtmatchlist[i] << endl;
	}*/
}

void Registration::cal_recall_precision()
{
	vector<double> precision;
	vector<double> recall;
	precision.resize(iteration_number);
	recall.resize(iteration_number);

	for (int i = 0; i < iteration_number; i++)
	{
		int a = 0;
		int b = 0;
		int c = 0;
		for (int j = 0; j < gtmatchlist.size(); j++)
		{
			if ((matchlist[j][i] == gtmatchlist[j]) && gtmatchlist[j]>=0) a++;     //number of the iteration's match which is the same as ground truth's match
			if (gtmatchlist[j]>=0) b++;                                            //ground truth's match number 
			if (matchlist[j][i]>=0) c++;                                           //the iteration's match number 
		}
		precision[i] = 1.0* a / c ;
		recall[i]    = 1.0* a / b ;
	}
	
	ofstream ofs;
	ofs.open("precision and recall.txt");
	if (ofs.is_open())
	{
		
		ofs << "iteration No. " << "  precision  " << " recall   " << endl;
		for (int i = 0; i < iteration_number; i++)
		{
			ofs << i << "          "
				<< setiosflags(ios::fixed) << setprecision(3) << precision[i] << "         "
				<< setiosflags(ios::fixed) << setprecision(3) << recall[i] << endl;
		}
		ofs.close();
	}

}

void Registration::readGTRT()
{
	ifstream in("Final Rt.txt" , ios::in) ;
    if (!in)
    {
        return ;
    }
    double x1 = 0 , x2 = 0 , x3 = 0 , x4 =0 ;
    int i = 0 ;
    while (!in.eof())
    {
        in>>x1>>x2>>x3>>x4 ;
        if (in.fail())
        {
            break;
        }
        Rt_gt(i, 0) = x1;
		Rt_gt(i, 1) = x2;
		Rt_gt(i, 2) = x3;
		Rt_gt(i, 3) = x4;
        ++i ;
    }
	in.close();
	cout << "Load Ground Truth Rt ......" << endl;
	cout << "GT RT:" << endl << Rt_gt << endl;
}

double Registration::calCloudFeatureDistance(int cor_num){
	double cloudfeaturedistance;
	cloudfeaturedistance = 2.0*cor_num / (KP.kps_num + KP.kpt_num);
	return cloudfeaturedistance;
	//used for multi-view registration as the weight of MST
}