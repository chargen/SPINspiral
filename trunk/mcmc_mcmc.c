/* 
   
   SPINspiral:                parameter estimation on binary inspirals detected by LIGO, including spins of the binary members
   mcmc_mcmc.c:               routines that form the MCMC core of the code
   
   
   Copyright 2007, 2008, 2009 Christian Roever, Marc van der Sluys, Vivien Raymond, Ilya Mandel
   
   
   This file is part of SPINspiral.
   
   SPINspiral is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   SPINspiral is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with SPINspiral.  If not, see <http://www.gnu.org/licenses/>.
   
*/



#include <mcmc.h>


/**
 * \file mcmc_mcmc.c
 * \brief Contains MCMC routines
 */



// ****************************************************************************************************************************************************  
/**
 * \brief MCMC routine - forms the MCMC core of the program
 *
 * Initialise and build a Markov chain
 */
// ****************************************************************************************************************************************************  
void MCMC(struct runPar run, struct interferometer *ifo[])
{
  
  struct parset state;                        // MCMC/template parameter set struct
  
  // *** MCMC struct ***
  struct MCMCvariables mcmc;                  // MCMC variables struct
  mcmc.minlogL = 0.0;
  mcmc.minlogL = -1.e30;
  
  //Copy elements from run struct to mcmc struct:
  copyRun2MCMC(run, &mcmc);
  
  printf("\n  GPS base time:  %15d\n",(int)mcmc.baseTime);
  
  if(partemp==0) mcmc.nTemps=1;
  mcmc.ran = gsl_rng_alloc(gsl_rng_mt19937);  // GSL random-number seed
  gsl_rng_set(mcmc.ran, mcmc.seed);           // Set seed for this run
  
  
  char outfilename[99];
  mcmc.fouts = (FILE**)calloc(mcmc.nTemps,sizeof(FILE*));      // Sigma for correlated update proposals						     
  for(tempi=0;tempi<mcmc.nTemps;tempi++) {
    if(tempi==0 || savehotchains>0) {
      sprintf(outfilename,"mcmc.output.%6.6d.%2.2d",mcmc.seed,tempi);
      mcmc.fouts[tempi] = fopen(outfilename,"w");                       // In current dir, allows for multiple copies to run
    }
  }
  
  if(nburn0>=nburn) {
    //printf("\n   *** Warning: nburn0 > nburn, setting nburn0 = nburn*0.9 ***\n\n");
    nburn0 = (int)(0.9*(double)nburn);
  }
  
  
  // *** MEMORY ALLOCATION ********************************************************************************************************************************************************
  
  int i=0,j=0,j1=0,j2=0,iIter=0;
  double tempratio=1.0;
  
  
  //Allocate memory for (most of) the MCMCvariables struct
  allocateMCMCvariables(&mcmc);
  
  
  double **tempcovar;
  tempcovar = (double**)calloc(mcmc.nMCMCpar,sizeof(double*)); // A temp Cholesky-decomposed matrix
  for(i=0;i<mcmc.nMCMCpar;i++) {
    tempcovar[i] = (double*)calloc(mcmc.nMCMCpar,sizeof(double));
  }
  
  double ***covar1;
  covar1  = (double***)calloc(mcmc.nTemps,sizeof(double**)); // The actual covariance matrix
  for(i=0;i<mcmc.nTemps;i++) {
    covar1[i]  = (double**)calloc(mcmc.nMCMCpar,sizeof(double*));
    for(j=0;j<mcmc.nMCMCpar;j++) {
      covar1[i][j]  = (double*)calloc(mcmc.nMCMCpar,sizeof(double));
    }
  }
  
  
  
  
  
  // *** INITIALISE PARALLEL TEMPERING ********************************************************************************************************************************************
  
  // *** Set up temperature ladder ***
  if(mcmc.nTemps>1) {  
    tempratio = exp(log(tempmax)/(double)(mcmc.nTemps-1));
    if(prpartempinfo>0) {
      printf("   Temperature ladder:\n     Number of chains:%3d,  Tmax:%7.2lf, Ti/Ti-1:%7.3lf\n",mcmc.nTemps,tempmax,tempratio);
      if(partemp==1) printf("     Using fixed temperatures for the chains\n");
      if(partemp==2) printf("     Using sinusoid temperatures for the chains\n");
      if(partemp==3) printf("     Using a manual temperature ladder with fixed temperatures for the chains\n");
      if(partemp==4) printf("     Using a manual temperature ladder with sinusoid temperatures for the chains\n");
      printf("     Chain     To     Ampl.    Tmin     Tmax\n");
    }
    for(tempi=0;tempi<mcmc.nTemps;tempi++) {
      mcmc.temps[tempi] = pow(10.0,log10(tempmax)/(double)(mcmc.nTemps-1)*(double)tempi);
      if(partemp==3 || partemp==4) mcmc.temps[tempi] = run.temps[tempi];  //Set manual ladder
      
      //if(tempi>0) mcmc.tempAmpl[tempi] = (mcmc.temps[tempi] - mcmc.temps[tempi-1])/(tempratio+1.0)*tempratio;  //Temperatures of adjacent chains just touch at extrema (since in antiphase)
      //if(tempi>0) mcmc.tempAmpl[tempi] = 1.5*(mcmc.temps[tempi] - mcmc.temps[tempi-1])/(tempratio+1.0)*tempratio;  //Temperatures of adjacent chains overlap somewhat at extrema (since in antiphase)
      if(tempi>0)                    mcmc.tempAmpl[tempi] = min(3.0*(mcmc.temps[tempi] - mcmc.temps[tempi-1])/(tempratio+1.0)*tempratio , fabs(mcmc.temps[tempi]-mcmc.temps[tempi-1]));  //Temperatures of adjacent chains overlap a lot at extrema (since in antiphase), make sure Ti,min>=T-i1,0
      if(mcmc.nTemps>10 && tempi>1)  mcmc.tempAmpl[tempi] = min(3.0*(mcmc.temps[tempi] - mcmc.temps[tempi-1])/(tempratio+1.0)*tempratio , fabs(mcmc.temps[tempi]-mcmc.temps[tempi-2]));  //Temperatures of adjacent chains overlap a lot at extrema (since in antiphase), make sure Ti,min>=T-i1,0
      //if(tempi>0) mcmc.tempAmpl[tempi] = fabs(mcmc.temps[tempi]-mcmc.temps[tempi-1]);  //Temperatures of adjacent chains overlap: Amplitude = (T(i) - T(i-1))  (may be a bit smallish for large nTemps)
      if(tempi==0 || partemp<=1 || partemp==3) mcmc.tempAmpl[tempi] = 0.0;
      if(prpartempinfo>0) printf("     %3d  %7.2lf  %7.2lf  %7.2lf  %7.2lf\n",tempi,mcmc.temps[tempi],mcmc.tempAmpl[tempi],mcmc.temps[tempi]-mcmc.tempAmpl[tempi],mcmc.temps[tempi]+mcmc.tempAmpl[tempi]);
    }
    if(prpartempinfo>0) printf("\n\n");
  }
  if(mcmc.nTemps==1) mcmc.temps[0] = 1.0;
  tempi = 0;  //MUST be zero
  
  
  
  
  
  // *** WRITE RUN 'HEADER' TO SCREEN AND FILE ************************************************************************************************************************************
  
  // Write 'header' to screen and file
  writeMCMCheader(ifo, mcmc, run);
  tempi = 0;  //MUST be zero
  
  
  
  
  
  // *** INITIALISE MARKOV CHAIN **************************************************************************************************************************************************
  
  // *** Get true (or best-guess) values for signal ***
  getInjectionParameters(&state, mcmc.nMCMCpar, mcmc.injParVal);
  allocParset(&state, mcmc.networkSize);
  
  
  // *** Write true/best-guess values to screen and file ***
  par2arr(state, mcmc.param, mcmc);  //Put the variables in their array
  localPar(&state, ifo, mcmc.networkSize);
  mcmc.logL[tempi] = netLogLikelihood(&state, mcmc.networkSize, ifo, mcmc.mcmcWaveform);  //Calculate the likelihood
  
  mcmc.iIter = -1;
  mcmc.iTemp = tempi;
  for(tempi=0;tempi<mcmc.nTemps;tempi++) {
    mcmc.iTemp = tempi;
    for(j1=0;j1<mcmc.nMCMCpar;j1++) {
      mcmc.param[tempi][j1] = mcmc.param[0][j1];
    }
    mcmc.logL[tempi] = mcmc.logL[0];
    writeMCMCoutput(mcmc, ifo);  //Write output line with injection parameters to screen and/or file (iteration -1)
  }
  tempi = 0;  //MUST be zero
  
  
  //Determine the number of parameters that is actually fitted/varied (i.e. not kept fixed at the true values)
  for(i=0;i<mcmc.nMCMCpar;i++) {
    if(mcmc.parFix[i]==0) mcmc.nParFit += 1;
  }
  
  
  // *** Initialise covariance matrix (initially diagonal), to do updates in the first block ***
  mcmc.corrUpdate[0] = 0;
  if(corrupd>0) {
    for(j1=0;j1<mcmc.nMCMCpar;j1++) {
      mcmc.covar[tempi][j1][j1] = mcmc.parSigma[j1];
    }
    mcmc.corrUpdate[0] = 1; //Use the matrix above and don't change it
    if(corrupd==2) mcmc.corrUpdate[0] = 2; //Use the matrix above and update it every nCorr iterations
  }
  
  
  
  
  // ***  GET (OFFSET) STARTING VALUES  ***********************************************************************************************************************************************
  
  // Get the best-guess values for the chain:
  getStartParameters(&state, run);
  allocParset(&state, mcmc.networkSize);
  
  par2arr(state, mcmc.param, mcmc);  //Put the variables in their array
  startMCMCOffset(&state,&mcmc,ifo);  // Start MCMC offset if and where wanted
  
  
  // *** Set the NEW array, sigma and scale ***
  for(i=0;i<mcmc.nMCMCpar;i++) {
    mcmc.nParam[tempi][i] = mcmc.param[tempi][i];
    mcmc.sig[tempi][i]   = 0.1  * mcmc.parSigma[i];
    if(adapt==1) mcmc.sig[tempi][i] = mcmc.parSigma[i]; //Don't use adaptation (?)
    mcmc.scale[tempi][i] = 10.0 * mcmc.parSigma[i];
    //mcmc.scale[tempi][i] = 0.0 * mcmc.parSigma[i]; //No adaptation
    sigmaPeriodicBoundaries(mcmc.sig[tempi][i], i, mcmc);
  }
  
  
  
  
  
  // *** WRITE STARTING STATE TO SCREEN AND FILE **********************************************************************************************************************************
  
  arr2par(mcmc.param, &state, mcmc);                         //Get the parameters from their array
  localPar(&state, ifo, mcmc.networkSize);
  mcmc.logL[tempi] = netLogLikelihood(&state, mcmc.networkSize, ifo, mcmc.mcmcWaveform);  //Calculate the likelihood

  // *** Write output line to screen and/or file
  printf("\n");
  mcmc.iIter = 0;
  for(tempi=0;tempi<mcmc.nTemps;tempi++) {
    mcmc.iTemp = tempi;
    for(j1=0;j1<mcmc.nMCMCpar;j1++) {
      mcmc.param[tempi][j1] = mcmc.param[0][j1];
    }
    mcmc.logL[tempi] = mcmc.logL[0];
    writeMCMCoutput(mcmc, ifo);  //Write output line to screen and/or file
  }
  tempi = 0;  //MUST be zero
  
  
  
  
  
  // *** INITIALISE PARALLEL TEMPERING ********************************************************************************************************************************************
  
  // *** Put the initial values of the parameters, sigmas etc in the different temperature chains ***
  if(mcmc.nTemps>1) {
    for(tempi=1;tempi<mcmc.nTemps;tempi++) {
      for(j=0;j<mcmc.nMCMCpar;j++) {
	mcmc.param[tempi][j] = mcmc.param[0][j];
	mcmc.nParam[tempi][j] = mcmc.nParam[0][j];
	mcmc.sig[tempi][j] = mcmc.sig[0][j];
	mcmc.scale[tempi][j] = mcmc.scale[0][j];
	mcmc.logL[tempi] = mcmc.logL[0];
	mcmc.nlogL[tempi] = mcmc.nlogL[0];

	for(j1=0;j1<mcmc.nMCMCpar;j1++) {
	  for(j2=0;j2<=j1;j2++) {
	    mcmc.covar[tempi][j1][j2] = mcmc.covar[0][j1][j2];
	  }
	}
      }
      mcmc.corrUpdate[tempi] = mcmc.corrUpdate[0];
      //mcmc.corrUpdate[tempi] = 0; //Correlated update proposals only for T=1 chain?
      //mcmc.corrUpdate[mcmc.nTemps-1] = 0; //Correlated update proposals not for hottest chain
    }
  }
  
  
  
  
  
  
  
  
  // ********************************************************************************************************************************************************************************
  // ***  CREATE MARKOV CHAIN   *****************************************************************************************************************************************************
  // ********************************************************************************************************************************************************************************
  
  iIter = 1;
  while(iIter<=nIter) {  //loop over Markov-chain states 
    mcmc.iIter = iIter;
    
    for(tempi=0;tempi<mcmc.nTemps;tempi++) {  //loop over temperature ladder
      mcmc.iTemp = tempi;
      //printf(" %d  %d  %d\n",tempi,mcmc.nTemps,iIter);
      
      //Set temperature
      if(partemp==1 || partemp==3) { //Chains at fixed T
	mcmc.chTemp = mcmc.temps[tempi];
      }
      if(partemp==2 || partemp==4) { //Chains with sinusoid T
	if(tempi==0) {
	  mcmc.chTemp = 1.0;
	}
	else {
	  //mcmc.chTemp = mcmc.temps[tempi] * (1.0  +  0.5 * pow((-1.0),tempi) * sin(tpi*(double)iIter/((double)nCorr)));  //Sinusoid around the temperature T_i with amplitude 0.5*T_i and period nCorr
	  //mcmc.chTemp = mcmc.temps[tempi]  +  mcmc.tempAmpl[tempi] * pow((-1.0),tempi) * sin(tpi*(double)iIter/((double)nCorr));  //Sinusoid around the temperature T_i with amplitude tempAmpl and period nCorr
	  //mcmc.chTemp = mcmc.temps[tempi]  +  mcmc.tempAmpl[tempi] * pow((-1.0),tempi) * sin(tpi*(double)iIter/(0.5*(double)nCorr));  //Sinusoid around the temperature T_i with amplitude tempAmpl and period 1/2 nCorr
	  mcmc.chTemp = mcmc.temps[tempi]  +  mcmc.tempAmpl[tempi] * pow((-1.0),tempi) * sin(tpi*(double)iIter/(5.0*(double)nCorr));  //Sinusoid around the temperature T_i with amplitude tempAmpl and period 5 * nCorr
	}
	//printf("%4d  %10.3lf\n",tempi,mcmc.chTemp);
      }
      
      
      
      // *** UPDATE MARKOV CHAIN STATE **************************************************************************************************************************************************
      
      // *** Uncorrelated update *************************************************************************************************
      //if(mcmc.corrUpdate[tempi]<=0) {
      if(gsl_rng_uniform(mcmc.ran) > run.corrfrac) {                                               //Do correlated updates from the beginning (quicker, but less efficient start); this saves ~4-5h for 2D, nCorr=1e4, nTemps=5
	//if(gsl_rng_uniform(mcmc.ran) > run.corrfrac || iIter < nCorr) {                              //Don't do correlated updates in the first block; more efficient per iteration, not necessarily per second
	//if(iIter>nburn && gsl_rng_uniform(mcmc.ran) < run.blockfrac){                            //Block update: only after burnin
	if(gsl_rng_uniform(mcmc.ran) < run.blockfrac){                                             //Block update: always
	  uncorrelatedMCMCblockUpdate(ifo, &state, &mcmc);
	}
	else{                                                                                       //Componentwise update (e.g. 90% of the time)
	  uncorrelatedMCMCsingleUpdate(ifo, &state, &mcmc);
	}
      } //End uncorrelated update
      
      
      // *** Correlated update ****************************************************************************************************
      //if(mcmc.corrUpdate[tempi]>=1) {
      else {
	correlatedMCMCupdate(ifo, &state, &mcmc);
      }
      
      
      // Update the dlogL = logL - logLo, and remember the parameter values where it has a maximum
      mcmc.dlogL[tempi] = mcmc.logL[tempi];
      if(mcmc.dlogL[tempi]>mcmc.maxdlogL[tempi]) {
	mcmc.maxdlogL[tempi] = mcmc.dlogL[tempi];
	for(i=0;i<mcmc.nMCMCpar;i++) {
	  mcmc.maxParam[tempi][i] = mcmc.param[tempi][i];
	}
      }
      
      
      // *** ACCEPT THE PROPOSED UPDATE *************************************************************************************************************************************************
      
      if(mcmc.acceptPrior[0]==1) { //Then write output and care about the correlation matrix
	
	
	// *** WRITE STATE TO SCREEN AND FILE *******************************************************************************************************************************************
	
	writeMCMCoutput(mcmc, ifo);  //Write output line to screen and/or file
	
	
	
	// *** CORRELATION MATRIX *******************************************************************************************************************************************************
	
	//if(mcmc.corrUpdate[tempi]==2) {  //Calculate correlations only once
	if(mcmc.corrUpdate[tempi]>=2) { //Calculate correlations multiple times
	  
	  // *** Save state to calculate correlations ***
	  if(mcmc.iHist[tempi]<nCorr) {
	    for(j1=0;j1<mcmc.nMCMCpar;j1++){
	      mcmc.hist[tempi][j1][mcmc.iHist[tempi]] = mcmc.param[tempi][j1];
	    }
	    mcmc.iHist[tempi] += 1;
	  }
	  
	  
	  
	  // ***  Update covariance matrix  and  print parallel-tempering info  *************************************************************
	  if(mcmc.iHist[tempi]>=nCorr) {
	    
	    updateCovarianceMatrix(&mcmc);  // Calculate the new covariance matrix and determine whether the matrix should be updated
	    
	    
	    if(partemp>=1 && prpartempinfo>0) writeChainInfo(mcmc);  //Print info on the current (temperature) chain(s) to screen
	    
	    
	    mcmc.iHist[tempi] = 0;        //Reset history counter for the covariance matrix  This is also used for parallel tempering, that should perhaps get its own counter
	    
	    /* 
	       if( (prMatrixInfo>0 || prpartempinfo>0) && tempi==mcmc.nTemps-1 ) {
	       printf("\n\n");
	       //printf("\n%10s  %15s  %8s  %8s  %16s  %8s  %8s  %8s  %8s  %8s  %8s  %8s  %8s  %8s\n","cycle","logL","Mc","eta","tc","logdL","spin","kappa","RA","sindec","phase","sinthJ0","phiJ0","alpha");
	       printf("\n%9s %10s  %7s %7s %8s %6s %6s %6s %6s %6s %6s %6s %6s %6s\n","cycle","logL","Mc","eta","tc","logdL","spin","kappa","RA","sindec","phase","snthJ0","phiJ0","alpha");
	       }
	    */
	    
	  } //if(mcmc.iHist[tempi]>=nCorr)
	} //if(mcmc.corrUpdate[tempi]>=2)
	// *** END CORRELATION MATRIX *************************************************************
      
      } //if(mcmc.acceptPrior[tempi]==1)
      
    } // for(tempi=0;tempi<mcmc.nTemps;tempi++) {  //loop over temperature ladder
    
    
    
    
    
    
    // *** ANNEALING ****************************************************************************************************************************************************************
    
    //Doesn't work with parallel tempering.  Use only when not using parallel tempering (and of course, temp0>1)
    if(partemp==0 && temp0>1.0) mcmc.chTemp = annealTemperature(temp0, nburn, nburn0, iIter);
    
    
    // *** A test with adaptive parallel tempering was here.   
    // The commented-out, incomplete subroutine adaptive_parallel_tempering() was removed in rev.170
    
    
    
    
    // *** PARALLEL TEMPERING:  Swap states between T-chains *************************************************************************
    
    if(mcmc.acceptPrior[0]==1 && partemp>=1 && mcmc.nTemps>1) swapChains(&mcmc);
    
    
    if(mcmc.acceptPrior[0]==1)iIter++;
  } // while(iIter<=nIter) {  //loop over markov chain states 
  
  
  
  for(tempi=0;tempi<mcmc.nTemps;tempi++) {
    if(tempi==0 || savehotchains>0) {
      fclose(mcmc.fouts[tempi]);
    }
  }
  free(mcmc.fouts);
  
  
  // *** FREE MEMORY **************************************************************************************************************************************************************
  
  printf("\n");
  freeMCMCvariables(&mcmc);
  
  
  for(i=0;i<mcmc.nMCMCpar;i++) {
    free(tempcovar[i]);
  }
  free(tempcovar);
  
  for(i=0;i<mcmc.nTemps;i++) {
    for(j=0;j<mcmc.nMCMCpar;j++) {
      free(covar1[i][j]);
    }
    free(covar1[i]);
  }
  free(covar1);
  
  freeParset(&state);
  
} // End MCMC()
// ****************************************************************************************************************************************************  















// ****************************************************************************************************************************************************  
/**
 * \brief Put the MCMC parameters in an array
 *
 */
// ****************************************************************************************************************************************************  
void par2arr(struct parset par, double **param, struct MCMCvariables mcmc)
{
  int i;
  for(i=0;i<mcmc.nMCMCpar;i++){
    param[tempi][i] = par.par[i];
  }
} // End par2arr
// ****************************************************************************************************************************************************  



// ****************************************************************************************************************************************************  
/**
 * \brief Get the MCMC parameters out of their array
 *
 */
// ****************************************************************************************************************************************************  
void arr2par(double **param, struct parset *par, struct MCMCvariables mcmc)
{
  int i;
  for(i=0;i<mcmc.nMCMCpar;i++){
    par->par[i] = param[tempi][i];
  }
} // End arr2par
// ****************************************************************************************************************************************************  












  
// ****************************************************************************************************************************************************  
/**
 * \brief Compute the prior for the given parameter set
 *
 * Contains boundary conditions and prior information for the MCMC.  Try to avoid returning 0, to increase jump sizes
 */
// ****************************************************************************************************************************************************  
double prior(double *par, int p, struct MCMCvariables mcmc)
{
  double priorValue = 1.0;
  
  if(mcmc.priorType[p]==21) {                                               // Periodic boundary condition to bring the variable between 0 and 2pi
    *par = fmod(*par+mtpi,tpi); 
  } else if(mcmc.priorType[p]==22) {                                        // Periodic boundary condition to bring the variable between 0 and pi
    *par = fmod(*par+mtpi,pi); 
  } else {                                                                  // Bounce back from the wall
    if(*par < mcmc.priorBoundLow[p] || *par > mcmc.priorBoundUp[p]) {                                        // Do only one bounce
      if(*par < mcmc.priorBoundLow[p]) {
	*par = mcmc.priorBoundLow[p] + fabs(*par - mcmc.priorBoundLow[p]);
      } else {
	*par = mcmc.priorBoundUp[p] - fabs(*par - mcmc.priorBoundUp[p]);
      }
      if(*par<mcmc.priorBoundLow[p] || *par>mcmc.priorBoundUp[p]) priorValue = 0.0;                             // If, after bouncing once, still outside the range, reject
    }
  }
  
  return priorValue;
} // End prior
// ****************************************************************************************************************************************************  



// ****************************************************************************************************************************************************  
/**
 * \brief Bring the adaptation sigma between its periodic boundaries
 *
 */
// ****************************************************************************************************************************************************  
double sigmaPeriodicBoundaries(double sigma, int p, struct MCMCvariables mcmc)
{
  if(mcmc.priorType[p] == 21) {
    return min(tpi,sigma);                                     //Bring sigma between 0 and 2pi;
  } else if(mcmc.priorType[p] == 22) {
    return min(pi,sigma);                                      //Bring sigma between 0 and pi;
  } else {
    return sigma;                                              //Don't do anything
  }
} // End sigmaPeriodicBoundaries()
// ****************************************************************************************************************************************************  








// ****************************************************************************************************************************************************  
/**
 * \brief Do a correlated block MCMC update
 *
 * Do an update for all non-fixed MCMC parameters. Use the covariance matrix to take into account correlations. 
 * The covariance matrix has been constructed from previous iterations.
 */
// ****************************************************************************************************************************************************  
void correlatedMCMCupdate(struct interferometer *ifo[], struct parset *state, struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int p1=0, p2=0, tempi=mcmc->iTemp, tempj=0;
  double temparr[mcmc->nMCMCpar], dparam=0.0;
  double ran=0.0, largejump1=0.0, largejumpall=0.0;
  
  //Prepare the proposal by creating a vector of univariate gaussian random numbers
  largejumpall = 1.0;
  ran = gsl_rng_uniform(mcmc->ran);
  if(ran < 1.0e-3) {
    largejumpall = 1.0e1;    //Every 1e3 iterations, take a 10x larger jump in all parameters
    if(ran < 1.0e-4) largejumpall = 1.0e2;    //Every 1e4 iterations, take a 100x larger jump in all parameters
  }
  
  for(p1=0;p1<mcmc->nMCMCpar;p1++) {
    largejump1 = 1.0;
    ran = gsl_rng_uniform(mcmc->ran);
    if(ran < 1.0e-2) {
      largejump1 = 1.0e1;    //Every 1e2 iterations, take a 10x larger jump in this parameter
      if(ran < 1.0e-3) largejump1 = 1.0e2;    //Every 1e3 iterations, take a 100x larger jump in this parameter
    }
    tempj = tempi;
    /*
    if(largejump1*largejumpall > 1.01) { //When making a larger jump, use the 'hotter' covariance matrix
      tempj = min(tempj+1,mcmc->nTemps);
      if(largejump1*largejumpall > 10.01) tempj = min(tempj+1,mcmc->nTemps);
    }
    */
    temparr[p1] = gsl_ran_gaussian(mcmc->ran,1.0) * mcmc->corrSig[tempj] * largejump1 * largejumpall;   //Univariate gaussian random numbers, with sigma=1, times the adaptable sigma_correlation times the large-jump factors
  }
  
  //Do the proposal
  mcmc->acceptPrior[tempi] = 1;
  for(p1=0;p1<mcmc->nMCMCpar;p1++){
    if(mcmc->parFix[p1]==0) {
      dparam = 0.0;
      for(p2=0;p2<=p1;p2++){
	dparam += mcmc->covar[tempi][p1][p2]*temparr[p2];   //Temparr is now a univariate gaussian random vector
      }
      mcmc->nParam[tempi][p1] = mcmc->param[tempi][p1] + dparam;       //Jump from the previous parameter value
      mcmc->sigOut[tempi][p1] = fabs(dparam);                          //This isn't really sigma, but the proposed jump size
      mcmc->acceptPrior[tempi] *= (int)prior(&mcmc->nParam[tempi][p1],p1,*mcmc);
   }
  }
  
  
  
  /*
  //Testing with sky position/orientation updates
  if(gsl_rng_uniform(mcmc->ran) < 0.33) mcmc->nParam[tempi][6]  = fmod(mcmc->nParam[tempi][6]+pi,tpi);  //Move RA over 12h
  if(gsl_rng_uniform(mcmc->ran) < 0.33) mcmc->nParam[tempi][7]  *= -1.0;                                //Flip declination
  if(gsl_rng_uniform(mcmc->ran) < 0.33) mcmc->nParam[tempi][9]  *= -1.0;                                //Flip theta_Jo
  if(gsl_rng_uniform(mcmc->ran) < 0.33) mcmc->nParam[tempi][10] = fmod(mcmc->nParam[tempi][10]+pi,tpi); //Move phi_Jo over 12h
  */

  
  
  
  //Decide whether to accept
  if(mcmc->acceptPrior[tempi]==1) {                                    //Then calculate the likelihood
    arr2par(mcmc->nParam, state, *mcmc);	                               //Get the parameters from their array
    localPar(state, ifo, mcmc->networkSize);
    mcmc->nlogL[tempi] = netLogLikelihood(state, mcmc->networkSize, ifo, mcmc->mcmcWaveform); //Calculate the likelihood
    par2arr(*state, mcmc->nParam, *mcmc);	                               //Put the variables back in their array
    
    if(exp(max(-30.0,min(0.0,mcmc->nlogL[tempi]-mcmc->logL[tempi]))) > pow(gsl_rng_uniform(mcmc->ran),mcmc->chTemp) && mcmc->nlogL[tempi] > mcmc->minlogL) {  //Accept proposal
      for(p1=0;p1<mcmc->nMCMCpar;p1++){
	if(mcmc->parFix[p1]==0) {
	  mcmc->param[tempi][p1] = mcmc->nParam[tempi][p1];
	  mcmc->accepted[tempi][p1] += 1;
	}
      }
      mcmc->logL[tempi] = mcmc->nlogL[tempi];
      if(adapt==1){ 
	mcmc->corrSig[tempi] *= 10.0;  //Increase sigma
      }
    }
    else {                                                      //Reject proposal because of low likelihood
      if(adapt==1){ 
	mcmc->corrSig[tempi] *= 0.5;  //Decrease sigma
      }
    }
  }
  else {                                                      //Reject proposal because of boundary conditions.  Perhaps one should increase the step size, or at least not decrease it?
    /*
      if(adapt==1){ 
      mcmc->corrSig[tempi] *= 0.8;
      }
    */
  }
} // End correlatedMCMCupdate
// ****************************************************************************************************************************************************  












// ****************************************************************************************************************************************************  
/**
 * \brief Do an uncorrelated, per-parameter MCMC update
 *
 * Do an update for all non-fixed MCMC parameters. Propose a jump and decide whether to accept or not on a per-parameter basis.
 */
// ****************************************************************************************************************************************************  
void uncorrelatedMCMCsingleUpdate(struct interferometer *ifo[], struct parset *state, struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int p=0, tempi=mcmc->iTemp;
  double gamma=0.0,alphastar=0.25;
  double ran=0.0, largejump1=0.0, largejumpall=0.0;
  
  largejumpall = 1.0;
  ran = gsl_rng_uniform(mcmc->ran);
  if(ran < 1.0e-3) largejumpall = 1.0e1;    //Every 1e3 iterations, take a 10x larger jump in all parameters
  if(ran < 1.0e-4) largejumpall = 1.0e2;    //Every 1e4 iterations, take a 100x larger jump in all parameters
  
  for(p=0;p<mcmc->nMCMCpar;p++){
    if(mcmc->parFix[p]==0) mcmc->nParam[tempi][p] = mcmc->param[tempi][p];
  }
  for(p=0;p<mcmc->nMCMCpar;p++){
    if(mcmc->parFix[p]==0) {
      largejump1 = 1.0;
      ran = gsl_rng_uniform(mcmc->ran);
      if(ran < 1.0e-2) largejump1 = 1.0e1;    //Every 1e2 iterations, take a 10x larger jump in this parameter
      if(ran < 1.0e-3) largejump1 = 1.0e2;    //Every 1e3 iterations, take a 100x larger jump in this parameter
      
      mcmc->nParam[tempi][p] = mcmc->param[tempi][p] + gsl_ran_gaussian(mcmc->ran,mcmc->sig[tempi][p]) * largejump1 * largejumpall;
      
      /*
      //Testing with sky position/orientation updates
      if(p==6  && gsl_rng_uniform(mcmc->ran) < 0.3) mcmc->nParam[tempi][6]  = fmod(mcmc->nParam[tempi][6]+pi,tpi);  //Move RA over 12h
      if(p==7  && gsl_rng_uniform(mcmc->ran) < 0.3) mcmc->nParam[tempi][7]  *= -1.0;                                //Flip declination
      if(p==9  && gsl_rng_uniform(mcmc->ran) < 0.3) mcmc->nParam[tempi][9]  *= -1.0;                                //Flip theta_Jo
      if(p==10 && gsl_rng_uniform(mcmc->ran) < 0.3) mcmc->nParam[tempi][10] = fmod(mcmc->nParam[tempi][10]+pi,tpi); //Move phi_Jo over 12h
      */
      
      mcmc->acceptPrior[tempi] = (int)prior(&mcmc->nParam[tempi][p],p,*mcmc);
      
      if(mcmc->acceptPrior[tempi]==1) {
	arr2par(mcmc->nParam, state, *mcmc);                                            //Get the parameters from their array
	localPar(state, ifo, mcmc->networkSize);
	mcmc->nlogL[tempi] = netLogLikelihood(state, mcmc->networkSize, ifo, mcmc->mcmcWaveform);   //Calculate the likelihood
	par2arr(*state, mcmc->nParam, *mcmc);                                            //Put the variables back in their array
	
	if(exp(max(-30.0,min(0.0,mcmc->nlogL[tempi]-mcmc->logL[tempi]))) > pow(gsl_rng_uniform(mcmc->ran),mcmc->chTemp) && mcmc->nlogL[tempi] > mcmc->minlogL) {  //Accept proposal
	  mcmc->param[tempi][p] = mcmc->nParam[tempi][p];
	  mcmc->logL[tempi] = mcmc->nlogL[tempi];
	  if(adapt==1){
	    gamma = mcmc->scale[tempi][p]*pow(1.0/((double)(mcmc->iIter+1)),1.0/6.0);
	    mcmc->sig[tempi][p] = max(0.0,mcmc->sig[tempi][p] + gamma*(1.0 - alphastar)); //Accept
	    sigmaPeriodicBoundaries(mcmc->sig[tempi][p], p, *mcmc);  //Bring the sigma between 0 and 2pi
	  }
	  mcmc->accepted[tempi][p] += 1;
	}
	else{                                                      //Reject proposal
	  mcmc->nParam[tempi][p] = mcmc->param[tempi][p];
	  if(adapt==1){
	    gamma = mcmc->scale[tempi][p]*pow(1.0/((double)(mcmc->iIter+1)),1.0/6.0);
	    mcmc->sig[tempi][p] = max(0.0,mcmc->sig[tempi][p] - gamma*alphastar); //Reject
	    sigmaPeriodicBoundaries(mcmc->sig[tempi][p], p, *mcmc);  //Bring the sigma between 0 and 2pi
	    //mcmc->sig[tempi][p] = max(0.01*mcmc->sig[tempi][p], mcmc->sig[tempi][p] - gamma*alphastar);
	  }
	}
      }
      else{  //If new state not within boundaries
	mcmc->nParam[tempi][p] = mcmc->param[tempi][p];
	if(adapt==1) {
	  gamma = mcmc->scale[tempi][p]*pow(1.0/((double)(mcmc->iIter+1)),1.0/6.0);
	  mcmc->sig[tempi][p] = max(0.0,mcmc->sig[tempi][p] - gamma*alphastar); //Reject
	  sigmaPeriodicBoundaries(mcmc->sig[tempi][p], p, *mcmc);  //Bring the sigma between 0 and 2pi
	}
      } //if(mcmc->acceptPrior[tempi]==1)
    } //if(mcmc->parFix[p]==0)
    mcmc->sigOut[tempi][p] = mcmc->sig[tempi][p]; //Save sigma for output
  } //p
} // End uncorrelatedMCMCsingleUpdate
// ****************************************************************************************************************************************************  

	




// ****************************************************************************************************************************************************  
/**
 * \brief Do an uncorrelated block update
 *
 * Do an update for all non-fixed MCMC parameters. Propose a jump and decide whether to accept or not for all parameters at once.
 */
// ****************************************************************************************************************************************************  
void uncorrelatedMCMCblockUpdate(struct interferometer *ifo[], struct parset *state, struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int p=0;
  double ran=0.0, largejump1=0.0, largejumpall=0.0;
  
  largejumpall = 1.0;
  ran = gsl_rng_uniform(mcmc->ran);
  if(ran < 1.0e-3) largejumpall = 1.0e1;    //Every 1e3 iterations, take a 10x larger jump in all parameters
  if(ran < 1.0e-4) largejumpall = 1.0e2;    //Every 1e4 iterations, take a 100x larger jump in all parameters
  
  mcmc->acceptPrior[tempi] = 1;
  for(p=0;p<mcmc->nMCMCpar;p++){
    if(mcmc->parFix[p]==0) {
      largejump1 = 1.0;
      ran = gsl_rng_uniform(mcmc->ran);
      if(ran < 1.0e-2) largejump1 = 1.0e1;    //Every 1e2 iterations, take a 10x larger jump in this parameter
      if(ran < 1.0e-3) largejump1 = 1.0e2;    //Every 1e3 iterations, take a 100x larger jump in this parameter
  
      mcmc->nParam[tempi][p] = mcmc->param[tempi][p] + gsl_ran_gaussian(mcmc->ran,mcmc->sig[tempi][p]) * largejump1 * largejumpall;
      mcmc->acceptPrior[tempi] *= (int)prior(&mcmc->nParam[tempi][p],p,*mcmc);
    }
  }
  
  if(mcmc->acceptPrior[tempi]==1) {
    arr2par(mcmc->nParam, state, *mcmc);	                              //Get the parameters from their array
    localPar(state, ifo, mcmc->networkSize);                               //Calculate local variables
    mcmc->nlogL[tempi] = netLogLikelihood(state, mcmc->networkSize, ifo, mcmc->mcmcWaveform);  //Calculate the likelihood
    par2arr(*state, mcmc->nParam, *mcmc);	                              //Put the variables back in their array
    
    if(exp(max(-30.0,min(0.0,mcmc->nlogL[tempi]-mcmc->logL[tempi]))) > pow(gsl_rng_uniform(mcmc->ran),mcmc->chTemp) && mcmc->nlogL[tempi] > mcmc->minlogL){  //Accept proposal if L>Lo
      for(p=0;p<mcmc->nMCMCpar;p++){
	if(mcmc->parFix[p]==0) {
	  mcmc->param[tempi][p] = mcmc->nParam[tempi][p];
	  mcmc->accepted[tempi][p] += 1;
	}
      }
      mcmc->logL[tempi] = mcmc->nlogL[tempi];
    }
  }
} // End uncorrelatedMCMCblockUpdate
// ****************************************************************************************************************************************************  




















// ****************************************************************************************************************************************************  
/**
 * \brief Write MCMC header to screen and file
 *
 */
// ****************************************************************************************************************************************************  
void writeMCMCheader(struct interferometer *ifo[], struct MCMCvariables mcmc, struct runPar run)
// ****************************************************************************************************************************************************  
{
  int i=0, tempi=0;
  
  // *** Print run parameters to screen ***
  if(mcmc.offsetMCMC==0) printf("   Starting MCMC from the true initial parameters\n\n");
  if(mcmc.offsetMCMC>=1) printf("   Starting MCMC from offset initial parameters\n\n");
  
  // *** Open the output file and write run parameters in the header ***
  for(tempi=0;tempi<mcmc.nTemps;tempi++) {
    if(tempi==0 || savehotchains>0) {
      fprintf(mcmc.fouts[tempi], "%10s  %10s  %6s  %20s  %6s %8s   %6s  %8s  %10s  %12s\n","nIter","Nburn","seed","null likelihood","Ndet","nCorr","nTemps","Tmax","Tchain","Network SNR");
      
      fprintf(mcmc.fouts[tempi], "%10d  %10d  %6d  %20.10lf  %6d %8d   %6d%10d%12.1f%14.6f\n",nIter,nburn,mcmc.seed, 0.0 ,run.networkSize,nCorr,mcmc.nTemps,(int)tempmax,mcmc.temps[tempi],run.netsnr);
      fprintf(mcmc.fouts[tempi], "\n%16s  %16s  %10s  %10s  %10s  %10s  %20s  %15s  %12s  %12s  %12s\n",
	      "Detector","SNR","f_low","f_high","before tc","after tc","Sample start (GPS)","Sample length","Sample rate","Sample size","FT size");
      for(i=0;i<run.networkSize;i++) {
	fprintf(mcmc.fouts[tempi], "%16s  %16.8lf  %10.2lf  %10.2lf  %10.2lf  %10.2lf  %20.8lf  %15.7lf  %12d  %12d  %12d\n",
		ifo[i]->name,ifo[i]->snr,ifo[i]->lowCut,ifo[i]->highCut,ifo[i]->before_tc,ifo[i]->after_tc,
		ifo[i]->FTstart,ifo[i]->deltaFT,ifo[i]->samplerate,ifo[i]->samplesize,ifo[i]->FTsize);
      }
      
      fprintf(mcmc.fouts[tempi], "\n\n%10s %13s","cycle","logL");
      for(i=0;i<mcmc.nMCMCpar;i++) {
	if(mcmc.parID[i]>=11 && mcmc.parID[i]<=19) {  //GPS time
	  fprintf(mcmc.fouts[tempi], " %17s",mcmc.parAbrev[mcmc.parID[i]]);
	} else {
	  fprintf(mcmc.fouts[tempi], " %10s",mcmc.parAbrev[mcmc.parID[i]]);
	}
      }
      fprintf(mcmc.fouts[tempi],"\n");
      
      fflush(mcmc.fouts[tempi]);
    }
  }
} // End writeMCMCheader
// ****************************************************************************************************************************************************  




	

// ****************************************************************************************************************************************************  
/**
 * \brief Write an MCMC iteration as an output line to screen and/or file
 *
 */
// ****************************************************************************************************************************************************  
void writeMCMCoutput(struct MCMCvariables mcmc, struct interferometer *ifo[])
// ****************************************************************************************************************************************************  
{
  int p=0, tempi=mcmc.iTemp, iIter=mcmc.iIter, i=0;
  
  //printf("%d  %d",tempi,iIter);
  
  // *** Write output to screen ***
  if(tempi==0) { //Only for the T=1 chain
    /*ILYA*/
    // if((iIter % (50*thinScreenOutput))==0 || iIter<0) printf("Previous iteration has match of %10g with true signal\n\n", 
    //	matchBetweenParameterArrayAndTrueParameters(mcmc.param[tempi], ifo, mcmc); //CHECK need support for two different waveforms
    // While the above is commented out, get rid of 'not used' warnings for the ifo struct:
    ifo[0]->index = ifo[0]->index;
    
    if((iIter % (50*thinScreenOutput))==0 || iIter<0) {
      printf("\n%9s%10s","cycle","logL");
      for(i=0;i<mcmc.nMCMCpar;i++) {
	if(mcmc.parID[i]>=11 && mcmc.parID[i]<=19) {  //GPS time
	  printf(" %18s",mcmc.parAbrev[mcmc.parID[i]]);
	} else {
	  printf(" %9s",mcmc.parAbrev[mcmc.parID[i]]);
	}
      }
      printf("\n");
    }
    
    
    if((iIter % thinScreenOutput)==0 || iIter<0){  printf("%9d%10.3lf",iIter,mcmc.logL[tempi]);
      for(i=0;i<mcmc.nMCMCpar;i++) {
	if(mcmc.parID[i]>=11 && mcmc.parID[i]<=19) {  //GPS time
	  printf(" %18.4f",mcmc.param[tempi][i]);
	} else {
	  printf(" %9.4f",mcmc.param[tempi][i]);
	}
      }
      printf("\n");}
    
    
  }
  
  
  double *accrat;
  accrat = (double*)calloc(mcmc.nMCMCpar,sizeof(double));
  for(p=0;p<mcmc.nMCMCpar;p++) {
    accrat[p] = 0.0;
  }
  if(iIter > 0) {
    for(p=0;p<mcmc.nMCMCpar;p++) {
      accrat[p] = mcmc.accepted[tempi][p]/(double)iIter;
    }
  }
  
  // *** Write output to file ***
  if(tempi==0 || savehotchains>0) { //For all T-chains if desired, otherwise the T=1 chain only
    if((iIter % thinOutput)==0 || iIter<=0){
      if(iIter<=0 || tempi==0 || (iIter % (thinOutput*savehotchains))==0) { //Save every thinOutput-th line for the T=1 chain, but every (thinOutput*savehotchains)-th line for the T>1 ones
	fprintf(mcmc.fouts[tempi], "%10d %13.6lf", iIter,mcmc.logL[tempi]);
	for(i=0;i<mcmc.nMCMCpar;i++) {
	  if(mcmc.parID[i]>=11 && mcmc.parID[i]<=19) {  //GPS time
	    fprintf(mcmc.fouts[tempi]," %17.6f",mcmc.param[tempi][i]);
	  } else {
	    fprintf(mcmc.fouts[tempi]," %10.6f",mcmc.param[tempi][i]);
	  }
	}
	fprintf(mcmc.fouts[tempi],"\n");
	
	
	fflush(mcmc.fouts[tempi]); //Make sure any 'snapshot' you take halfway is complete
	
      } //if(tempi==0 || (iIter % (thinOutput*savehotchains))==0)
    } //if((iIter % thinOutput)==0 || iIter<0)
  } //if(tempi==0)
  
  free(accrat);
} // End writeMCMCoutput
// ****************************************************************************************************************************************************  









// ****************************************************************************************************************************************************  
/**
 * \brief Allocate memory for the MCMCvariables struct.
 *
 * Allocate memory for the MCMCvariables struct.  Don't forget to deallocate whatever you put here in freeMCMCvariables()
 */
// ****************************************************************************************************************************************************  
void allocateMCMCvariables(struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int i=0, j=0;
  
  mcmc->nParFit=0;
  
  mcmc->histMean  = (double*)calloc(mcmc->nMCMCpar,sizeof(double));     // Mean of hist block of iterations, used to get the covariance matrix
  mcmc->histDev = (double*)calloc(mcmc->nMCMCpar,sizeof(double));       // Standard deviation of hist block of iterations, used to get the covariance matrix
  for(i=0;i<mcmc->nMCMCpar;i++) {
    mcmc->histMean[i] = 0;
    mcmc->histDev[i] = 0;
  }
  
  mcmc->corrUpdate = (int*)calloc(mcmc->nTemps,sizeof(int));
  mcmc->acceptElems = (int*)calloc(mcmc->nTemps,sizeof(int));
  for(i=0;i<mcmc->nTemps;i++) {
    mcmc->corrUpdate[i] = 0;
    mcmc->acceptElems[i] = 0;
  }
  
  mcmc->temps = (double*)calloc(mcmc->nTemps,sizeof(double));        // Array of temperatures in the temperature ladder								
  mcmc->newTemps = (double*)calloc(mcmc->nTemps,sizeof(double));     // New temperature ladder, was used in adaptive parallel tempering						
  mcmc->tempAmpl = (double*)calloc(mcmc->nTemps,sizeof(double));     // Temperature amplitudes for sinusoid T in parallel tempering							
  mcmc->logL = (double*)calloc(mcmc->nTemps,sizeof(double));         // Current log(L)												
  mcmc->nlogL = (double*)calloc(mcmc->nTemps,sizeof(double));        // New log(L)													
  mcmc->dlogL = (double*)calloc(mcmc->nTemps,sizeof(double));        // log(L)-log(Lo)												
  mcmc->maxdlogL = (double*)calloc(mcmc->nTemps,sizeof(double));     // Remember the maximum dlog(L)											
  for(i=0;i<mcmc->nTemps;i++) {
    mcmc->temps[i] = 0.0;
    mcmc->newTemps[i] = 0.0;
    mcmc->tempAmpl[i] = 0.0;
    mcmc->logL[i] = 0.0;
    mcmc->nlogL[i] = 0.0;
    mcmc->dlogL[i] = 0.0;
    mcmc->maxdlogL[i] = -1.e30;
  }
  
  mcmc->corrSig = (double*)calloc(mcmc->nTemps,sizeof(double));      // Sigma for correlated update proposals						     
  mcmc->swapTs1 = (int*)calloc(mcmc->nTemps,sizeof(int));	           // Totals for the columns in the chain-swap matrix					     
  mcmc->swapTs2 = (int*)calloc(mcmc->nTemps,sizeof(int));	           // Totals for the rows in the chain-swap matrix                                              
  mcmc->acceptPrior = (int*)calloc(mcmc->nTemps,sizeof(int));        // Check boundary conditions and choose to accept (1) or not(0)				     
  mcmc->iHist = (int*)calloc(mcmc->nTemps,sizeof(int));	           // Count the iteration number in the current history block to calculate the covar matrix from
  for(i=0;i<mcmc->nTemps;i++) {
    mcmc->corrSig[i] = 1.0;
    mcmc->swapTs1[i] = 0;
    mcmc->swapTs2[i] = 0;
    mcmc->acceptPrior[i] = 1;
    mcmc->iHist[i] = 0;
  }
  
  mcmc->accepted = (int**)calloc(mcmc->nTemps,sizeof(int*));         // Count accepted proposals
  mcmc->swapTss = (int**)calloc(mcmc->nTemps,sizeof(int*));          // Count swaps between chains
  mcmc->param = (double**)calloc(mcmc->nTemps,sizeof(double*));      // The old parameters for all chains
  mcmc->nParam = (double**)calloc(mcmc->nTemps,sizeof(double*));     // The new parameters for all chains
  mcmc->maxParam = (double**)calloc(mcmc->nTemps,sizeof(double*));   // The best parameters for all chains (max logL)
  mcmc->sig = (double**)calloc(mcmc->nTemps,sizeof(double*));        // The standard deviation of the gaussian to draw the jump size from
  mcmc->sigOut = (double**)calloc(mcmc->nTemps,sizeof(double*));     // The sigma that gets written to output
  mcmc->scale = (double**)calloc(mcmc->nTemps,sizeof(double*));      // The rate of adaptation
  for(i=0;i<mcmc->nTemps;i++) {
    mcmc->accepted[i] = (int*)calloc(mcmc->nMCMCpar,sizeof(int));
    mcmc->swapTss[i] = (int*)calloc(mcmc->nTemps,sizeof(int));
    mcmc->param[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    mcmc->nParam[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    mcmc->maxParam[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    mcmc->sig[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    mcmc->sigOut[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    mcmc->scale[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
  }
  
  mcmc->hist    = (double***)calloc(mcmc->nTemps,sizeof(double**));  // Store a block of iterations, to calculate the covariances
  mcmc->covar  = (double***)calloc(mcmc->nTemps,sizeof(double**));   // The Cholesky-decomposed covariance matrix
  for(i=0;i<mcmc->nTemps;i++) {
    mcmc->hist[i]    = (double**)calloc(mcmc->nMCMCpar,sizeof(double*));
    mcmc->covar[i]  = (double**)calloc(mcmc->nMCMCpar,sizeof(double*));
    for(j=0;j<mcmc->nMCMCpar;j++) {
      mcmc->hist[i][j]    = (double*)calloc(nCorr,sizeof(double));
      mcmc->covar[i][j]  = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    }
  }  
} // End allocateMCMCvariables
// ****************************************************************************************************************************************************  




// ****************************************************************************************************************************************************  
/**
 * \brief Deallocate memory for the MCMCvariables struct
 *
 */
// ****************************************************************************************************************************************************  
void freeMCMCvariables(struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int i=0, j=0;
  gsl_rng_free(mcmc->ran);
  
  free(mcmc->histMean);
  free(mcmc->histDev);
  
  free(mcmc->corrUpdate);
  free(mcmc->acceptElems);
  
  free(mcmc->temps);
  free(mcmc->newTemps);
  free(mcmc->tempAmpl);
  free(mcmc->logL);
  free(mcmc->nlogL);
  free(mcmc->dlogL);
  free(mcmc->maxdlogL);
  
  free(mcmc->corrSig);
  free(mcmc->acceptPrior);
  free(mcmc->iHist);
  free(mcmc->swapTs1);
  free(mcmc->swapTs2);
  
  for(i=0;i<mcmc->nTemps;i++) {
    free(mcmc->accepted[i]);
    free(mcmc->swapTss[i]);
    free(mcmc->param[i]);
    free(mcmc->nParam[i]);
    free(mcmc->maxParam[i]);
    free(mcmc->sig[i]);
    free(mcmc->sigOut[i]);
    free(mcmc->scale[i]);
  }
  free(mcmc->accepted);
  free(mcmc->swapTss);
  free(mcmc->param);
  free(mcmc->nParam);
  free(mcmc->maxParam);
  free(mcmc->sig);
  free(mcmc->sigOut);
  free(mcmc->scale);
  
  for(i=0;i<mcmc->nTemps;i++) {
    for(j=0;j<mcmc->nMCMCpar;j++) {
      free(mcmc->hist[i][j]);
      free(mcmc->covar[i][j]);
    }
    free(mcmc->hist[i]);
    free(mcmc->covar[i]);
  }
  free(mcmc->hist);
  free(mcmc->covar);
} // End freeMCMCvariables
// ****************************************************************************************************************************************************  












// ****************************************************************************************************************************************************  
/**
 * \brief Calculate the new covariance matrix and determine whether the new matrix should be accepted
 *
 */
// ****************************************************************************************************************************************************  
void updateCovarianceMatrix(struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int i=0, j=0, i1=0, p1=0, p2=0;
  double **tempcovar;
  tempcovar = (double**)calloc(mcmc->nMCMCpar,sizeof(double*));           // A temp Cholesky-decomposed matrix
  for(i=0;i<mcmc->nMCMCpar;i++) {
    tempcovar[i] = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
  }
  double ***covar1;
  covar1  = (double***)calloc(mcmc->nTemps,sizeof(double**));   // The actual covariance matrix
  for(i=0;i<mcmc->nTemps;i++) {
    covar1[i]  = (double**)calloc(mcmc->nMCMCpar,sizeof(double*));
    for(j=0;j<mcmc->nMCMCpar;j++) {
      covar1[i][j]  = (double*)calloc(mcmc->nMCMCpar,sizeof(double));
    }
  }
  
  
  //Calculate the mean
  for(p1=0;p1<mcmc->nMCMCpar;p1++){
    mcmc->histMean[p1]=0.0;
    for(i1=0;i1<nCorr;i1++){
      mcmc->histMean[p1]+=mcmc->hist[tempi][p1][i1];
    }
    mcmc->histMean[p1]/=((double)nCorr);
  }
  
  //Calculate the standard deviation. Only for printing, not used in the code
  for(p1=0;p1<mcmc->nMCMCpar;p1++){
    mcmc->histDev[p1]=0.0;
    for(i1=0;i1<nCorr;i1++){
      mcmc->histDev[p1] += (mcmc->hist[tempi][p1][i1]-mcmc->histMean[p1])*(mcmc->hist[tempi][p1][i1]-mcmc->histMean[p1]);
    }
    mcmc->histDev[p1] = sqrt(mcmc->histDev[p1]/(double)(nCorr-1));
  }
  
  //Calculate the covariances and save them in covar1
  for(p1=0;p1<mcmc->nMCMCpar;p1++){
    for(p2=0;p2<=p1;p2++){
      covar1[tempi][p1][p2]=0.0;
      for(i1=0;i1<nCorr;i1++){
	covar1[tempi][p1][p2] += (mcmc->hist[tempi][p1][i1] - mcmc->histMean[p1]) * (mcmc->hist[tempi][p2][i1] - mcmc->histMean[p2]);
      }
      covar1[tempi][p1][p2] /= (double)(nCorr-1);
    }
  }
  
  //Store the covariance matrix in a temporary variable tempcovar, for Cholesky decomposition
  for(p1=0;p1<mcmc->nMCMCpar;p1++){
    for(p2=0;p2<=p1;p2++){
      tempcovar[p1][p2] = covar1[tempi][p1][p2];
    }
  }
  
  if(tempi==0 && prMatrixInfo>0) printf("\n\n");
  
  //Do Cholesky decomposition
  CholeskyDecompose(tempcovar,mcmc);
  
  //Get conditions to decide whether to accept the new matrix or not
  mcmc->acceptElems[tempi] = 0;
  for(p1=0;p1<mcmc->nMCMCpar;p1++) {
    if(mcmc->parFix[p1]==0) {
      if(tempcovar[p1][p1] < mcmc->covar[tempi][p1][p1]) mcmc->acceptElems[tempi] += 1; //Smaller diagonal element is better; count for how many this is the case
      if(tempcovar[p1][p1]<=0.0 || isnan(tempcovar[p1][p1])!=0 || isinf(tempcovar[p1][p1])!=0) mcmc->acceptElems[tempi] -= 9999;  //If diagonal element is <0, NaN or Inf
    }
  }
  mcmc->acceptElems[tempi] = max(mcmc->acceptElems[tempi],-1); //Now -1 means there is a diagonal element that is 0, NaN or Inf
  
  //Print matrix information  
  if(prMatrixInfo==2 && tempi==0){
    printf("\n  Update for the covariance matrix proposed at iteration:  %10d\n",mcmc->iIter);
    printf("\n    AcceptElems: %d\n",mcmc->acceptElems[tempi]);
    printf("\n    Covariance matrix:\n");
    for(p1=0;p1<mcmc->nMCMCpar;p1++){
      for(p2=0;p2<=p1;p2++){
	printf("    %10.3g",covar1[tempi][p1][p2]);
      }
      printf("\n");
    }
    printf("\n    Old Cholesky-decomposed matrix:\n");
    for(p1=0;p1<mcmc->nMCMCpar;p1++){
      for(p2=0;p2<=p1;p2++){
	printf("    %10.3g",mcmc->covar[tempi][p1][p2]);
      }
      printf("\n");
    }
    printf("\n    New Cholesky-decomposed matrix:\n");
    for(p1=0;p1<mcmc->nMCMCpar;p1++){
      for(p2=0;p2<=p1;p2++){
	//for(p2=0;p2<mcmc->nMCMCpar;p2++){
	printf("    %10.3g",tempcovar[p1][p2]);
      }
      printf("\n");
    }
  }
  
  // Copy the new covariance matrix from tempcovar into mcmc->covar
  if(mcmc->acceptElems[tempi]>=0) { //Accept new matrix only if no Infs, NaNs, 0s occur.
    if(mcmc->corrUpdate[tempi]<=2 || (double)mcmc->acceptElems[tempi] >= (double)mcmc->nParFit*mcmc->matAccFr) { //Always accept the new matrix on the first update, otherwise only if the fraction matAccFr of diagonal elements are better (smaller) than before
      for(p1=0;p1<mcmc->nMCMCpar;p1++){
	for(p2=0;p2<=p1;p2++){
	  mcmc->covar[tempi][p1][p2] = tempcovar[p1][p2]; 
	}
      }
      mcmc->corrUpdate[tempi] += 1;
      if(prMatrixInfo>0 && tempi==0) printf("  Proposed covariance-matrix update at iteration %d accepted.  AcceptElems: %d.  Accepted matrices: %d/%d \n", mcmc->iIter, mcmc->acceptElems[tempi], mcmc->corrUpdate[tempi]-2, (int)((double)mcmc->iIter/(double)nCorr));  // -2 since you start with 2
    }
    else {
      if(prMatrixInfo>0 && tempi==0) printf("  Proposed covariance-matrix update at iteration %d rejected.  AcceptElems: %d.  Accepted matrices: %d/%d \n", mcmc->iIter, mcmc->acceptElems[tempi], mcmc->corrUpdate[tempi]-2, (int)((double)mcmc->iIter/(double)nCorr));  // -2 since you start with 2
    }
  }
  else{
    if(prMatrixInfo>0 && tempi==0) printf("  Proposed covariance-matrix update at iteration %d rejected.  AcceptElems: %d.  Accepted matrices: %d/%d \n", mcmc->iIter, mcmc->acceptElems[tempi], mcmc->corrUpdate[tempi]-2, (int)((double)mcmc->iIter/(double)nCorr));  // -2 since you start with 2
  }
  
  
  
  //Deallocate memory
  for(i=0;i<mcmc->nMCMCpar;i++) {
    free(tempcovar[i]);
  }
  free(tempcovar);
  
  for(i=0;i<mcmc->nTemps;i++) {
    for(j=0;j<mcmc->nMCMCpar;j++) {
      free(covar1[i][j]);
    }
    free(covar1[i]);
  }
  free(covar1);
  
} // End updateCovarianceMatrix
// ****************************************************************************************************************************************************  
  
  





// ****************************************************************************************************************************************************  
/**
 * \brief Compute Cholesky decomposition matrix
 *
 * Performs Cholesky decompositon of matrix A and returns result in the same matrix - adapted from PJG Fortran function
 * If matrix is not positive definite, return zeroes
 */
// ****************************************************************************************************************************************************  
void CholeskyDecompose(double **A, struct MCMCvariables *mcmc)
{
  int j1=0,j2=0,j3=0,notposdef=0;
  int n=mcmc->nMCMCpar;
  double sum=0.0;
  for(j1=0;j1<n;j1++){
    if(mcmc->parFix[j1]==0) {
      sum = A[j1][j1];
      for(j2=0;j2<j1;j2++){
	if(mcmc->parFix[j2]==0) {
	  sum -= A[j1][j2]*A[j1][j2];
	}
      }
      if(sum<0.0) {
	notposdef=1;
      }
      else {
	A[j1][j1]=sqrt(sum);
	for(j2=j1+1;j2<n;j2++){
	  if(mcmc->parFix[j2]==0){
	    sum = A[j2][j1];
	    for(j3=0;j3<j1;j3++){
	      if(mcmc->parFix[j3]==0){
		sum -= A[j2][j3]*A[j1][j3];
	      }
	    }
	  }
	  A[j2][j1] = sum/A[j1][j1];
	}
      }
    }
  }
  if(notposdef==1) {
    //printf("  CholeskyDecompose(): Matrix %d is not positive definite\n",tempi);
    for(j1=0;j1<n;j1++){
      for(j2=0;j2<n;j2++){
	A[j1][j2] = 0.0;
      }
    }
  }
} // End CholeskyDecompose()
// ****************************************************************************************************************************************************  










// ****************************************************************************************************************************************************  
/**
 * \brief Annealing: set the temperature according to the iteration number and burnin
 */
// ****************************************************************************************************************************************************  
double annealTemperature(double temp0, int nburn, int nburn0, int iIter)
// ****************************************************************************************************************************************************  
{
  double temperature = 1.0;
  //temperature = temp0*pow(1.0/((double)(max(iIter,nCorr))),1.0/10.0);
  //temperature = temp0*pow(((double)(iIter)),-0.1);
  //temperature = temp0*pow(((double)(iIter)),-0.25);
  //temperature = temp0 * pow(((double)(iIter)), (-log(temp0)/log((double)nburn)) );  //Temp drops from temp0 to 1.0 in nburn iterations
  //printf("%f\n",-log(temp0)/log((double)nburn));
  //temperature = min( max( temp0*pow( max((double)iIter-0.1*(double)nburn,1.0) ,-log10(temp0)/log10(0.9*(double)nburn)) , 1.0) , temp0);  //Temp stays at temp0 for 10% of burnin and then drops to 1.0 at the end of the burnin (too drastic for short burnins/high percentages?)
  //temperature = temp0*pow( max((double)iIter-1000.0,1.0) ,-log10(temp0)/log10((double)nburn-1000.0));  //Temp stays at temp0 for the first 1000 iterations and then drops to 1.0 at the end of the burnin (too drastic for short burnins/high percentages?)
  temperature = exp(log(temp0) * ((double)(nburn-iIter)/((double)(nburn-nburn0))));
  
  //if(gsl_rng_uniform(mcmc.ran)<1.e-2 && iIter > nburn && mcmc.dlogL[tempi] < 0.95*mcmc.maxdlogL[tempi]){
  //if(iIter > nburn && mcmc.dlogL[tempi] < 0.85*mcmc.maxdlogL[tempi]){
  //  temp=pow(temp0,0.5);
  //  mcmc.corrSig[tempi] *= 10.0;
  //}
  
  temperature = min( max(temperature,1.0) , temp0);  //Just in case...
  return temperature;
} // End annealTemperature
// ****************************************************************************************************************************************************  









// ****************************************************************************************************************************************************  
/**
 * \brief Parallel tempering: Swap states between two chains
 */
// ****************************************************************************************************************************************************  
void swapChains(struct MCMCvariables *mcmc)
// ****************************************************************************************************************************************************  
{
  int i=0, tempi=0, tempj=0;
  double tmpdbl = 0.0;
  
  //Swap parameters and likelihood between any two chains
  for(tempi=0;tempi<mcmc->nTemps-1;tempi++) {
    for(tempj=tempi+1;tempj<mcmc->nTemps;tempj++) {
      
      if(exp(max(-30.0,min(0.0, (1.0/mcmc->temps[tempi]-1.0/mcmc->temps[tempj]) * (mcmc->logL[tempj]-mcmc->logL[tempi]) ))) > gsl_rng_uniform(mcmc->ran)) { //Then swap...
	for(i=0;i<mcmc->nMCMCpar;i++) {
	  tmpdbl = mcmc->param[tempj][i]; //Temp var
	  mcmc->param[tempj][i] = mcmc->param[tempi][i];
	  mcmc->param[tempi][i] = tmpdbl;
	}
	tmpdbl = mcmc->logL[tempj];
	mcmc->logL[tempj] = mcmc->logL[tempi];
	mcmc->logL[tempi] = tmpdbl;
	mcmc->swapTss[tempi][tempj] += 1;
	mcmc->swapTs1[tempi] += 1;
	mcmc->swapTs2[tempj] += 1;
      }
    } //tempj
  } //tempi
} // End swapChains
// ****************************************************************************************************************************************************  








// ****************************************************************************************************************************************************  
/**
 * \brief Parallel tempering: Print chain and swap info to screen
 */
// ****************************************************************************************************************************************************  
void writeChainInfo(struct MCMCvariables mcmc)
// ****************************************************************************************************************************************************  
{
  int tempi=mcmc.iTemp, p=0, t1=0, t2=0;
  double tmpdbl = 0.0;
  
  if(tempi==0) {
    printf("\n\n      Chain  log(T)   dlog(L)  AccEls AccMat     Swap  AccRat lgStdv:");
    for(p=0;p<mcmc.nMCMCpar;p++) {
      printf(" %6s",mcmc.parAbrv[mcmc.parID[p]]);
    }
    printf("\n");
  }
  
  printf("        %3d   %5.3f     %3d    %3d   %6.4f  %6.4f        ",
	 tempi,log10(mcmc.chTemp),mcmc.acceptElems[tempi],mcmc.corrUpdate[tempi]-2,  (double)mcmc.swapTs1[tempi]/(double)mcmc.iIter,(double)mcmc.accepted[tempi][0]/(double)mcmc.iIter);
  for(p=0;p<mcmc.nMCMCpar;p++) {
    tmpdbl = log10(mcmc.histDev[p]+1.e-30);
    if(tmpdbl<-9.99) tmpdbl = 0.0;
    printf(" %6.3f",tmpdbl);
  }
  printf("\n");
  
  
  if(prpartempinfo==2 && tempi==mcmc.nTemps-1) { //Print swap-rate matrix for parallel tempering
    printf("\n   Chain swap: ");
    for(t1=0;t1<mcmc.nTemps-1;t1++) {
      printf("  %6d",t1);
    }
    printf("   total\n");
    for(t1=1;t1<mcmc.nTemps;t1++) {
      printf("            %3d",t1);
      for(t2=0;t2<mcmc.nTemps-1;t2++) {
	if(t2<t1) {
	  printf("  %6.4f",(double)mcmc.swapTss[t2][t1]/(double)mcmc.iIter);
	} else {
	  printf("        ");
	}
      }
      printf("  %6.4f\n",(double)mcmc.swapTs2[t1]/(double)mcmc.iIter);
    }
    printf("          total");
    for(t1=0;t1<mcmc.nTemps-1;t1++) {
      printf("  %6.4f",(double)mcmc.swapTs1[t1]/(double)mcmc.iIter);
    }
    printf("\n");
  } //if(prpartempinfo==2 && tempi==mcmc.nTemps-1)
} // End writeChainInfo
// ****************************************************************************************************************************************************  
	      
	      









// ****************************************************************************************************************************************************  
/**
 * \brief Copy some of the elements of the struct runPar to the struct MCMCvariables
 */
// ****************************************************************************************************************************************************  
void copyRun2MCMC(struct runPar run, struct MCMCvariables *mcmc)
{
  int i=0,j=0;
  
  //Copy some global variables:
  mcmc->maxnPar = run.maxnPar;                          // Absolute maximum number of mcmc/template parameters allowed
  mcmc->parDBn = run.parDBn;                            // Number of elements in hardcoded parameter database
  mcmc->nMCMCpar = run.nMCMCpar;                        // Number of mcmc/template parameters
  mcmc->nInjectPar = run.nInjectPar;                    // Number of mcmc/template parameters
  mcmc->chTemp = max(temp0,1.0);                          // Current temperature
  
  mcmc->mcmcWaveform = run.mcmcWaveform;                // Waveform used as MCMC template
  mcmc->injectionWaveform = run.injectionWaveform;      // Waveform used as injection template
  mcmc->networkSize = run.networkSize;                  // Network size
  mcmc->seed = run.MCMCseed;                            // MCMC seed
  mcmc->nTemps = run.nTemps;                            // Size of temperature ladder
  mcmc->matAccFr = run.matAccFr;                        // Fraction of elements on the diagonal that must 'improve' in order to accept a new covariance matrix.
  mcmc->offsetMCMC = run.offsetMCMC;                    // Start MCMC offset (i.e., not from injection values) or not
  mcmc->offsetX = run.offsetX;                          // Start offset chains from a Gaussian distribution offsetX times wider than parSigma
  
  mcmc->baseTime = (double)((floor)(prior_tc_mean/100.0)*100);  //'Base' time, gets rid of the first 6-7 digits of GPS time
  
  
  for(i=0;i<mcmc->maxnPar;i++) {
    mcmc->injParVal[i] = run.injParVal[i];
    mcmc->injID[i] = run.injID[i];
    
    mcmc->parNumber[i] = run.parNumber[i];
    mcmc->parID[i] = run.parID[i];
    mcmc->parBestVal[i] = run.parBestVal[i];
    mcmc->parFix[i] = run.parFix[i];
    mcmc->parStartMCMC[i] = run.parStartMCMC[i];
    mcmc->parSigma[i] = run.parSigma[i];
    
    mcmc->priorBoundLow[i] = run.priorBoundLow[i];
    mcmc->priorBoundUp[i] = run.priorBoundUp[i];
    mcmc->priorType[i] = run.priorType[i];
    
    //mcmc->[i] = run.[i];
  }
  
  //Parameter database:
  for(i=0;i<mcmc->parDBn;i++) {
    mcmc->injRevID[i] = run.injRevID[i];
    mcmc->parRevID[i] = run.parRevID[i];
    
    for(j=0;j<99;j++) {
      mcmc->parName[i][j] = run.parName[i][j];
      mcmc->parAbrev[i][j] = run.parAbrev[i][j];
      mcmc->parAbrv[i][j] = run.parAbrv[i][j];
    }
  }
} // End copyRun2MCMC()
// ****************************************************************************************************************************************************  





  
// ****************************************************************************************************************************************************  
/**
 * \brief Choose and print offset starting values for the Markov chain
 * 
 * Set MCMC parameters to either the best-guess values or the injection values (where possible).
 * Then, depending on the detailed per-parameter settings, add a random offset from this value.
 * Require a good value for logL (determined by mcmc->minlogL) in order to accept the proposed starting values, unless MCMC parameters do not fully match the injection parameters.
 * This happens independently of whether parameters are fixed or not!
 * Finally, print the selected starting values to screen.
 */
// ****************************************************************************************************************************************************  
void startMCMCOffset(struct parset *par, struct MCMCvariables *mcmc, struct interferometer *ifo[])
{
  int i=0, iInj=0, nstart=0, nDiffPar=0;
  double db = 0.0;
  
  printf("\n");
  mcmc->logL[tempi] = -9999.999;
  
  
  // *** Set each MCMC parameter to either the best-guess value or the injection value, depending on the per-parameter settings - nothing random about this bit
  for(i=0;i<mcmc->nMCMCpar;i++) {
    
    //Start at or around BestValue:
    if(mcmc->parStartMCMC[i]==1 || mcmc->parStartMCMC[i]==2) mcmc->nParam[tempi][i] = mcmc->parBestVal[i];
    
    //Start at or around the injection value where possible:
    if(mcmc->offsetMCMC == 0 || mcmc->parStartMCMC[i]==3 || mcmc->parStartMCMC[i]==4) {
      iInj = mcmc->injRevID[mcmc->parID[i]];  //Get the index of this parameter in the injection set.  -1 if not available.
      if(iInj >= 0) {
	mcmc->nParam[tempi][i] = mcmc->injParVal[iInj];  //Start at or around the injection value
      } else {
	mcmc->nParam[tempi][i] = mcmc->parBestVal[i];  //Start at or around BestValue - this should only happen if the injection waveform has different parameters than the MCMC waveform
	nDiffPar += 1;
      }
    }
    
    mcmc->param[tempi][i] = mcmc->nParam[tempi][i];
  }
  printf("\n");
  
  
  // Safety check:
  if(mcmc->mcmcWaveform == mcmc->injectionWaveform && nDiffPar != 0) 
    fprintf(stderr,"   startMCMCoffset:  WARNING:  The injection and MCMC waveform are identical, but %i different parameters were counted !!!\n",nDiffPar);
  
  
  
  // *** Add a random offset to the MCMC starting parameters:
  if(mcmc->offsetMCMC != 0) {
    while(mcmc->logL[tempi] < mcmc->minlogL+1.0) { // Accept only good starting values
      mcmc->acceptPrior[tempi] = 1;
      
      for(i=0;i<mcmc->nMCMCpar;i++) {  //For each MCMC parameter
	if(mcmc->parStartMCMC[i]==2 || mcmc->parStartMCMC[i]==4 || mcmc->parStartMCMC[i]==5) {  //Then find random offset parameters
	  
	  if(mcmc->parStartMCMC[i]==2 || mcmc->parStartMCMC[i]==4) {
	    mcmc->param[tempi][i] = mcmc->nParam[tempi][i] + mcmc->offsetX * gsl_ran_gaussian(mcmc->ran, mcmc->parSigma[i]);  //Gaussian with width parSigma around either Injection or BestValue
	  } else if(mcmc->parStartMCMC[i]==5) {
	    db = mcmc->priorBoundUp[i]-mcmc->priorBoundLow[i];                                     // Width of range
	    mcmc->param[tempi][i] = mcmc->priorBoundLow[i] + gsl_rng_uniform(mcmc->ran)*db;        // Draw random number uniform on range with width db
	  }
	  mcmc->acceptPrior[tempi] *= (int)prior(&mcmc->param[tempi][i],i,*mcmc);
	  
	} // if(mcmc->parStartMCMC[i]==2 || mcmc->parStartMCMC[i]==4 || mcmc->parStartMCMC[i]==5) {  //Then find random offset parameters
      } //i
      
      if(mcmc->acceptPrior[tempi]==1) {                     //Check the value of the likelihood for this draw
	arr2par(mcmc->param, par, *mcmc);	                      //Get the parameters from their array
	localPar(par, ifo, mcmc->networkSize);
	mcmc->logL[tempi] = netLogLikelihood(par, mcmc->networkSize, ifo, mcmc->mcmcWaveform);  //Calculate the likelihood
      }
      nstart = nstart + 1;
      
      
      
      // Print each trial starting value:
      if(printMuch>=1 && (nstart % thinOutput)==0) {
	printf("%9d%10.3lf",nstart,mcmc->logL[tempi]);
	for(i=0;i<mcmc->nMCMCpar;i++) {
	  if(mcmc->parID[i]>=11 && mcmc->parID[i]<=19) {  //GPS time
	    printf(" %18.4f",mcmc->param[tempi][i]);
	  } else {
	    printf(" %9.4f",mcmc->param[tempi][i]);
	  }
	}
	printf("\n");
      }
      
      if(mcmc->mcmcWaveform != mcmc->injectionWaveform && nDiffPar != 0) break;  //Don't require good logL if not all parameters match between injection and MCMC waveforms
      
    }  //while(mcmc->logL[tempi] < mcmc->minlogL+1.0) // Accept only good starting values
  } //if(mcmc->offsetMCMC != 0)
  
  
  
  // *** Print selected starting parameters to screen:
  //Print parameter names:
  printf("%9s%10s", "nDraws","logL");
  for(i=0;i<mcmc->nMCMCpar;i++) {
    if(mcmc->parID[i]>=11 && mcmc->parID[i]<=19) {  //GPS time
      printf(" %18s",mcmc->parAbrev[mcmc->parID[i]]);
    } else {
      printf(" %9s",mcmc->parAbrev[mcmc->parID[i]]);
    }
  }
  printf("\n");
  
  //Print parameter values:
  printf("%9d%10.3lf",nstart,mcmc->logL[tempi]);
  for(i=0;i<mcmc->nMCMCpar;i++) {
    if(mcmc->parID[i]>=11 && mcmc->parID[i]<=19) {  //GPS time
      printf(" %18.4f",mcmc->param[tempi][i]);
    } else {
      printf(" %9.4f",mcmc->param[tempi][i]);
    }
  }
  printf("\n");
  
} // End void startOffset()
// ****************************************************************************************************************************************************  





