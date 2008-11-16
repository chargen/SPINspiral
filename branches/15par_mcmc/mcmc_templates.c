#include <mcmc.h>


void template(struct parset *par, struct interferometer *ifo[], int ifonr)
// Call a waveform template, the global variable waveformversion determines which one
{
  if(waveformversion==1) {
    template12(par, ifo, ifonr);  // Apostolatos 12-parameter template
  } else if(waveformversion==2) {
    template15(par, ifo, ifonr);  // LAL 15-parameter template
  }
}




void template12(struct parset *par, struct interferometer *ifo[], int ifonr)
// Simplified spinning template in restricted 1.5PN order with 1 spin (Apostolatos)
//  The output vector `output' is of length `length',  starting at `tstart' and with resolution `samplerate'.
{
  //if(MvdSdebug) printf("      Template for ifo %d\n", ifonr);
  double x;
  double m1=0.0,m2=0.0,M=0.0,mu=0.0;
  double cvec1[3],cvec2[3],cvec3[3];
  double tvec1[3],tvec2[3],tvec4[3],tvec6[3],tvec7[3];
  int i, terminate=0;
  double n_L[3];
  double localtc=0.0,altitude=0.0,azimuth=0.0,spin=0.0,samplerate=0.0,inversesamplerate=0.0;
  int length=0;
  for(i=0;i<3;i++) {
    cvec1[i] = 0.0;
    cvec2[i] = 0.0;
    cvec3[i] = 0.0;
    tvec1[i] = 0.0;
    tvec2[i] = 0.0;
    tvec4[i] = 0.0;
    tvec6[i] = 0.0;
    tvec7[i] = 0.0;
    n_L[i] = 0.0;
  }
  
  localtc    = par->loctc[ifonr];
  altitude   = par->localti[ifonr];
  azimuth    = par->locazi[ifonr];
  samplerate = (double)ifo[ifonr]->samplerate;
  inversesamplerate = 1.0/samplerate;
  length     = ifo[ifonr]->samplesize;
  
  //printf("\n\n  %20.5lf  %f    %f  %f  %f    %f  %f  %f\n\n\n",par->tc,GMST(par->tc),par->longi*r2h,rightAscension(par->longi,GMST(par->tc))*r2h,asin(par->sinlati)*r2d, par->locazi[ifonr]*r2d,fmod(pi-(par->locazi[ifonr]+ifo[ifonr]->rightarm)+mtpi,tpi)*r2d,(pi/2.0-par->localti[ifonr])*r2d);
  
  
  double n_z[3] = {0.0,0.0,1.0};                                                                                        //North in global coordinates
  double normalvec[3];                                                                                                  
  for(i=0;i<3;i++) normalvec[i] = ifo[ifonr]->normalvec[i];                                                             //Detector position normal vector = local zenith vector z'
  double D_L = exp(par->logdl)*Mpcs;                                                                                    //Source luminosity distance, in seconds
  double coslati = sqrt(1.0-par->sinlati*par->sinlati);
  double n_N[3] = { cos(par->longi)*coslati , sin(par->longi)*coslati , par->sinlati };                                 //n_N: Position unit vector = N^
  
  double sthJ0   = par->sinthJ0;                                                                                        //n_J0: 'total' AM unit vector, J0^  (almost equal to the real J, see Eq.15)
  double cthJ0   = sqrt(1.0 - sthJ0*sthJ0);
  double n_J0[3] = { cos(par->phiJ0)*cthJ0 , sin(par->phiJ0)*cthJ0 , sthJ0 };                                           //Here, theta_Jo is a latitude-like angle like Dec (-pi/2-pi/2).
  
  par->NdJ = dotproduct(n_N,n_J0);                                                                                      //Inclination of J_0; only for printing purposes, should be removed from this routine
  
  //Get masses from Mch and eta
  double root = sqrt(0.25-par->eta);
  double fraction = (0.5-root) / (0.5+root);
  double inversefraction = 1.0/fraction;
  double Mc = par->mc*M0;                                                                                               //Chirp mass in seconds
  x = exp(0.6*log(fraction));
  m1 = Mc * (pow(1.0+fraction,0.2) / x);
  m2 = Mc * (pow(1.0+inversefraction,0.2) * x);
  M = m1+m2;                                                                                                            // Eq.16a
  mu = m1*m2/M;                                                                                                         // Eq.16b
  spin = par->spin*m1*m1;
  
  
  
  //if(printmuch) {printf("Ms: eta: %g  Mc: %g  m1: %g  m2: %g  M: %g  mu: %g  Mo: %g\n",par->eta,Mc/M0,m1/M0,m2/M0,M/M0,mu/M0,M0);}
  //printf("  %d  %lf  %lf  %lf  %lf  %d\n",ifonr,localtc,altitude,azimuth,samplerate,length);
  //printf("  %lf  %lf  %lf  %lf  %lf  %lf  %lf  %lf  %lf  %lf  %lf  %lf",
  // par->mc,par->eta,par->tc,par->logdl,par->spin,par->kappa,par->longi,par->sinlati,par->phase,par->sinthJ0,par->phiJ0,par->alpha);
  
  double beta = 1.0/12.0*(113.0*(m1*m1)/(M*M) + 75.0*par->eta)*par->kappa*spin/(m1*m1);                                 // Eq.20, for S2=0 or m1=m2,S1=S2:  kappa*spin/(m1*m1) = L^.S/m1^2, see Blanchet et al., PRL 74, 3515, 1995
  
  double cst1 = 743.0/336.0 + 11.0/4.0*par->eta;
  double cst2 = (4.0*pi-beta);
  double cst5 = spin*sqrt(1.0-par->kappa*par->kappa);
  
  //Constant vector 1 for the construction of Eq.41e
  facvec(n_J0,-sthJ0,tvec1);                                                                                             //tvec1 = -J0^*cos(theta_J0)   MvdS: theta_J0 is a latitude, not a co-latitude
  addvec(n_z,tvec1,tvec2);                                                                                               //tvec2 = n_z - J0^*cos(theta_J0)
  facvec(tvec2,1.0/cthJ0,cvec1);                                                                                         //cvec1 = (n_z - J0^*cos(theta_J0))/sin(theta_J0)
  
  //Constant vector 2 for the construction of Eq.41e
  crossproduct(n_J0,n_z,tvec1);                                                                                          //tvec1 = J0^ x z^
  facvec(tvec1,1.0/cthJ0,cvec2);                                                                                         //cvec2 = (J0^ x z^) / sin(theta_J0)
  
  //Constant vector 3 for the construction of Eq.12 (local polarisation for F+,x)
  facvec(n_N,-dotproduct(normalvec,n_N),tvec1);                                                                          //tvec1 = -N^(z^'.N^)
  addvec(normalvec,tvec1,cvec3);                                                                                         //cvec3 = z^' - N^(z^'.N^)
  
  //Construct Eq.8ab, needed for F+,Fx
  double cosalti   = cos(altitude);
  double sin2azi   = sin(2.0*azimuth);
  double cos2azi   = cos(2.0*azimuth);
  double cst6  = 0.5*(1.0+cosalti*cosalti)*cos2azi;
  double cst7  = cosalti*sin2azi;
  
  
  double omega_low  = pi*ifo[ifonr]->lowCut;   //30 or 40 Hz, translated from f_gw to omega_orb
  double omega_high = pi*ifo[ifonr]->highCut;  //1600 Hz, translated from f_gw to omega_orb
  //double omega_high = min(pi*ifo[ifonr]->highCut, exp(-1.5*log(cutoff_a) - log(M)) );  //1600 Hz, translated from f_gw to omega_orb, or a/M = cutoff_a, whichever is smaller
  
  /*
  if(printmuch) {
    double Tcoal = 5.0*pow(8.0*omega_low,-8.0*c3rd)*pow(Mc,-5.0*c3rd) * (1.0 + 4.0*c3rd*cst1*pow(omega_low*M,2.0*c3rd) - 1.6*cst2*(omega_low*M));   //Time between f_low and coalescence
    double t0 = localtc - Tcoal;
    double deltat = (double)length*inversesamplerate;
    printf("Times:  Tcoal: %g,  t0: %g,  localtc: %g,  length: %d,  dt: %g,  dt-ltc: %g\n",Tcoal,t0,localtc,length,deltat,deltat-localtc);
  }
  */
  
  double oldomega = -1.e30;
  double phi_gw=0.0,alpha=0.0;
  
  //To display the number of wave and precession cycles:
  double phi1 = 0.0;
  //double phi2 = 0.0;
  double alpha1 = 0.0;
  //double alpha2 = 0.0;
  int i1=0,i2=0;
  
  double t=0.0,tau=0.0,tau18=0.0,tau28=0.0,tau38=0.0,tau58=0.0,tau_18=0.0,tau_28=0.0,tau_38=0.0,tau_58=0.0,tau_68=0.0;
  double omega_orb=0.0,l_L=0.0,Y=0.0,Gsq=0.0,G=0.0,slamL=0.0,clamL=0.0,LdotN=0.0;
  double hplus=0.0,hcross=0.0,locpolar=0.0,sin2polar=0.0,cos2polar=0.0,Fplus=0.0,Fcross=0.0;
  double cst4=0.0,x1=0.0,x2=0.0,x3=0.0;
  double taperx[length],omegas[length];
  for(i=0;i<length;i++) {
    taperx[i] = 0.0;
    omegas[i] = 0.0;
  }
  
  // Fill 'output' with time-domain template:
  for (i=0; i<length; ++i){
    // Determine time left until coalescence, "(t_c-t)" in (4.17)/(11):
    t = localtc - ((double)i)*inversesamplerate;  // (time to t_c) = "(t_c-t)" in (4.17)
    if(t<0.0) { 
      //terminate = 1;
      if(terminate==0) terminate = 1;  //Set to 1 only if it was 0
    }
    else {
      tau    = par->eta/(5.0*M)*t;   //t = localtc-t already
      tau18  = exp(0.125*log(tau));  //tau^(1/8)
      tau28  = tau18*tau18;
      tau38  = tau28*tau18;
      tau58  = tau28*tau38;
      tau_18 = 1.0/tau18;            //tau^(-1/8)
      tau_28 = tau_18*tau_18;
      tau_38 = tau_18*tau_28;
      tau_58 = tau_28*tau_38;
      tau_68 = tau_38*tau_38;
      
      omega_orb = 1.0/(8.0*M) * (tau_38 + 0.125*cst1*tau_58 - 0.075*cst2*tau_68);   // Orbital frequency
      omegas[i] = omega_orb;
      //printf("  t: %lf  tau: %lf  omega_orb: %lf\n",t,tau,omega_orb);
    }
    
    if ((omega_orb>=omega_low) && (terminate==0)) {  // After source comes into window, before tc and before frequency reaches its maximum  Careful, if t<0, omega = nan!!!
      
      if (omega_orb < oldomega || omega_orb >= omega_high){  // Frequency starts decreasing, or frequency higher than highCut --> terminate signal
      //if (omega_orb < oldomega || omega_orb >= omega_high || taperx[i]>0.09){  // Frequency starts decreasing, or frequency higher than highCut, or v_orb>0.3c --> terminate signal
        ifo[ifonr]->FTin[i] = 0.0; 
        if(omega_orb < oldomega) terminate = 2;
        if(omega_orb >= omega_high) terminate = 3;
	//if(taperx[i]>0.09) terminate = 4;
      }
      
      else {             // Frequency still increasing --> keep on computing...
	if(i1==0) i1=i;  //Save initial i for tapering the beginning of the signal
	i2 = i;          //Save final i for tapering the end of the signal
        oldomega = omega_orb;
	taperx[i] = exp(2.0*c3rd*log(M*omega_orb));                                                                      // x := (M*w)^(2/3)  =  v_orb^2
        
        //Compute orbital A.M.
        l_L = m1*m2*exp(-c3rd*log(omega_orb*M));
        
        //GW and orbital phase
        phi_gw = par->phase - 2.0/par->eta * (tau58 + 0.625*c3rd*cst1*tau38 - 0.1875*cst2*tau28);                       // GW phase
	if(fabs(phi1)<1.e-30) phi1 = phi_gw;   //Save initial phi
	//phi2 = phi_gw;                       //Save final phi
        
        Y = spin/l_L;                                                                                                    //Y = |S|/|L|, Eq.43
        Gsq = 1.0 + 2.0*par->kappa*Y + Y*Y;                                                                                      //G^2, Eq.46
        G   = sqrt(Gsq);
        
        cst4 = l_L + par->kappa*spin;
        x = mu*M;
        x1 = x*x*x;
        x = G*l_L;
        x2 = x*x*x;
        x3 = spin*spin*spin;
        alpha = par->alpha - 5.0/(96.0*x1) * (1.0+0.75*m2/m1) * 
	  (2.0*x2 - 3.0*par->kappa*spin*cst4*G*l_L - 3.0*par->kappa*x3*(1.0-par->kappa*par->kappa) * asinh(cst4/cst5));                                            //Eq.47
	if(fabs(alpha1)<1.e-30) alpha1 = alpha;  //Save initial alpha
	//alpha2 = alpha;                         //Save final alpha
	
        slamL = cst5/(l_L*G);                                                                                            //sin(lambda_L), Eq.48a
        clamL = cst4/(l_L*G);                                                                                            //cos(lambda_L), Eq.48b
        
        
        //Construct Eq.41e
        facvec(n_J0,clamL,tvec1);                                                                                        //tvec1 = J0^*cos(lambda_L)
        facvec(cvec1,slamL*cos(alpha),tvec4);                                                                            //tvec4 = (n_z - J0^*cos(theta_J0))*sin(lambda_L)*cos(alpha)/sin(theta_J0)
        facvec(cvec2,slamL*sin(alpha),tvec6);                                                                            //tvec6 = (J0^ x z^) * sin(lambds_L)*sin(alpha)/sin(theta_J0)
        addvec(tvec1,tvec4,tvec7);                                                                                       //Construct Eq.59
        addvec(tvec7,tvec6,n_L);                                                                                         //Eq.59: n_L=L^
	
        
        LdotN  = dotproduct(n_L,n_N);                                                                                    //L^.N^
	x1     = 2.0*exp(5.0*c3rd*log(Mc))/D_L;
        x3     = exp(2.0*c3rd*log(omega_orb));
        hplus  =      x1 * (1.0 + LdotN*LdotN) * x3 * cos(phi_gw);
        hcross = -2.0*x1 * LdotN               * x3 * sin(phi_gw);
        
        
        //Local polarisation vector, F+,Fx:
        crossproduct(n_L,normalvec,tvec1);                                                                              //tvec1 = n_L x z^'
        locpolar  = atan(dotproduct(n_L,cvec3)/dotproduct(n_N,tvec1));                                                  //Eq.12 of Vecchio, result between -pi/2 and pi/2
        sin2polar = sin(2.0*locpolar);
	cos2polar = sqrt(1.0-sin2polar*sin2polar);                                                                      //Since 2*locpolar should be between -pi and pi (?)
        Fplus     =  cst6*cos2polar + cst7*sin2polar;                                                                   //Eq.8a
        Fcross    = -cst6*sin2polar + cst7*cos2polar;                                                                   //Eq.8b
        
	//Detector signal:
	ifo[ifonr]->FTin[i] = Fplus*hplus + Fcross*hcross;                                                              //  (3.10)
	
	
	
	//Print some stuff for diagnostics:
	//if((omega_orb/pi<40.002 || fabs(t)<0.2) && printmuch) {
	//if(printmuch) {
	//printf("i: %8d   t: %10g   f: %10g   x: %10g\n",i,t,omega_orb/pi,taperx[i]);
	//printf("omg_orb: %10g  phi_orb: %10g  l_L: %10g  S: %10g  k: %10g  Y: %10g  G: %10g  alpha: %10g  slamL: %10g  clamL: %10g \n",  omega_orb,phi_orb,l_L,spin,par->kappa,Y,G,alpha,slamL,clamL);
	//printf("i: %8d   t: %10g  alpha_c: %10g   alpha0: %10g   alpha: %10g  alpha/2pi: %10g\n",  i,t,par->alpha,alpha0,alpha,alpha/tpi);
	//}
	
	
      }
    }  //end if ((omega_orb>=omega_low) && (terminate==0)) {  // After source comes into window, before tc and before frequency reaches its maximum  Careful, if t<0, omega = nan!!!
    else {
      ifo[ifonr]->FTin[i]   = 0.0;  //  (after t_c or after termination)
    }
  }  //i
  
  
  
  //Print some stuff for diagnostics
  //if(i1<=1 && par->mc>0.02)  printf("   **********    Warning: length is too small to fit waveform template, increase before_tc    **********\n"); //Don't print when doing null-likelihood
  //if(i2>=length-1 && par->mc>0.02)  printf("   **********    Warning: length is too small to fit waveform template, increase after_tc    **********\n"); //Don't print when doing null-likelihood
  //if(printmuch) 
  //printf("%10.2f  %10.2f  %10.2f  %10.1f  %10.2f  %10.2f",par->spin,acos(par->kappa)*r2d,(double)(i2-i1)*inversesamplerate,(phi2-phi1)/tpi,(alpha2-alpha1)/tpi,pow(M*oldomega,-2.0*c3rd));
  //if(printmuch) printf("  term: %d  i1: %d  i2: %d  length: %d  f_gw,old: %lf  f_gw: %lf  f_gw,low: %lf  f_gw,high: %lf  f_gw1: %lf  f_gw2: %lf\n",terminate,i1,i2,length,oldomega/pi,omega_orb/pi,omega_low/pi,omega_high/pi,omegas[i1]/pi,omegas[i2]/pi);
  //Terminate: 1: t>tc, 2: df/dt<0, 3: f>f_high
  
  
  
  //Apply tapering
  //int i1a = i1 + ceil(2.0*samplerate*pi/omegas[i1]);  //pi/omega_orb = 1/f_gw = lambda_gw.  *samplerate: number of points in first wavelength
  int i2a = i2 - (int)ceil(2.0*samplerate*pi/omegas[i2]);  //pi/omega_orb = 1/f_gw = lambda_gw.  *samplerate: number of points in last wavelength
  for (i=i1;i<=i2;i++) {
    //ifo[ifonr]->FTin[i] *= 0.5*(1.0 - tanh(15000.0*(taperx[i1a]-taperx[i])));  //Taper beginning of template
    ifo[ifonr]->FTin[i] *= 0.5*(1.0 - tanh(100.0*(taperx[i]-taperx[i2a])));  //Taper end of template
  }
}
//End template12()












void localpar(struct parset *par, struct interferometer *ifo[], int networksize)
// Calculate the local parameters from the global ones, for the spinning parameters:
//    par   :  pointer to parameter set (struct)
//    ifo   :  pointer to interferometer data (struct)
{
  int i,j;
  double lineofsight[3], dummyvec[3], scalprod1, delay;
  
  // Determine local coalescence times:
  coord2vec(par->sinlati, par->longi, lineofsight);
  for (i=0; i<networksize; i++){
    scalprod1 =  ifo[i]->positionvec[0]*lineofsight[0]  +  ifo[i]->positionvec[1]*lineofsight[1]  +  ifo[i]->positionvec[2]*lineofsight[2];  // Project line of sight onto positionvec, scalprod1 is in units of metres
    delay = scalprod1 / c;                                         // Time delay (wrt geocentre) in seconds
    par->loctc[i] = ((par->tc - ifo[i]->FTstart) - delay);
  }
  
  // Determine local sky position:
  for (i=0; i<networksize; i++){
    // 'Altitude' in the ifo' frame:
    par->localti[i] = angle(ifo[i]->normalvec, lineofsight);       // Actually, this is the colatitude in the ifo' frame (i.e. 0deg=zenith, 90deg=horizon)
    
    // 'Azimuth' in the ifo' frame:
    for (j=0; j<3; ++j) dummyvec[j] = lineofsight[j];              // Temp vector with line of sight
    orthoproject(dummyvec, ifo[i]->rightvec, ifo[i]->orthoarm);    // Project line of sight into ifo' arm plane
    par->locazi[i] = angle(dummyvec, ifo[i]->rightvec);            // The 'true' azimuth (N=0,E=90deg) of the source at the location of the detector is:  pi - (par->locazi[i] + ifo[i]->rightarm) 
    if (!righthanded(ifo[i]->rightvec, dummyvec, ifo[i]->normalvec)) par->locazi[i] = 2.0*pi - par->locazi[i];
    
    //printf("  %d  %lf  %lf  %s\n",i,ifo[i]->lati/pi*180.0,ifo[i]->longi/pi*180.0,ifo[i]->name);
  }
  return;
}

