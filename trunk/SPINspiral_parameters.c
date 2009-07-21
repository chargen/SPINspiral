/* 
   
   SPINspiral:                parameter estimation on binary inspirals detected by LIGO, including spins of the binary members
   SPINspiral_parameters.c:   routines to read/write input files, set constants and set true and null parameters
   
   
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


#include <getopt.h>

#include <lal/LIGOMetadataTables.h>
#include <lal/LIGOLwXMLRead.h>

#include <SPINspiral.h>





/**
 * \file SPINspiral_parameters.c
 * \brief Contains routines that handle reading and randomisation of parameters
 */



// ****************************************************************************************************************************************************  
/** 
 * \brief Read and parse command-line arguments/options
 */
// ****************************************************************************************************************************************************  
void readCommandLineOptions(int argc, char* argv[], struct runPar *run)
{
  int c;
  if(argc > 1) printf("   Parsing %i command-line arguments:\n",argc-1);
  int i = 0;
  for (i=0;i<20;i++) {
	  run->commandflag[i] = 0;
  }
  
  //Set up struct with long (--) options:
  static struct option long_options[] =
    {
      {"injXMLfile",      required_argument, 0,               0},
      {"injXMLnr",        required_argument, 0,               0},
	  {"mChirp",          required_argument, 0,             'm'},
	  {"eta",             required_argument, 0,             'e'},
	  {"tc",              required_argument, 0,             't'},
	  {"dist",            required_argument, 0,             'd'},
	  {"nIter",           required_argument, 0,             'n'},
	  {"nSkip",           required_argument, 0,             's'},
	  {"nDet",            required_argument, 0,             'a'},
	  {"downsample",      required_argument, 0,             'a'},
	  {"beforetc",        required_argument, 0,             'a'},
	  {"aftertc",         required_argument, 0,             'a'},
	  {"Flow",            required_argument, 0,             'a'},
	  {"Fhigh",           required_argument, 0,             'a'},
      {"nPSDsegment",     required_argument, 0,             'a'},
      {"lPSDsegment",     required_argument, 0,             'a'},
		
		{"H1Strain",        required_argument, 0,             'a'},
		{"H1size",          required_argument, 0,             'a'},
		{"H1GPSstart",      required_argument, 0,             'a'},
		{"H1prefix",        required_argument, 0,             'a'},
		{"L1Strain",        required_argument, 0,             'a'},
		{"L1size",          required_argument, 0,             'a'},
		{"L1GPSstart",      required_argument, 0,             'a'},
		{"L1prefix",        required_argument, 0,             'a'},
		{"V1Strain",        required_argument, 0,             'a'},
		{"V1size",          required_argument, 0,             'a'},
		{"V1GPSstart",      required_argument, 0,             'a'},
		{"V1prefix",        required_argument, 0,             'a'},
		
	
	  {"datapath",        required_argument, 0,             'a'},
		
      {0, 0, 0, 0}
    };
  
  
  int option_index = 0;
  while( (c = getopt_long(argc, argv, "i:m:e:t:d:n:d:a:",long_options, &option_index)) != -1) {
    switch(c) {
      
      
      // *** Treat (untranslated) long options:
    case 0:
      if(strcmp(long_options[option_index].name,"injXMLfile")==0) {
	run->injXMLfilename=(char*)malloc(strlen(optarg)+1);
	strcpy(run->injXMLfilename,optarg);
	printf("    - reading injection parameters from the XML file %s\n",run->injXMLfilename);
      }
      if(strcmp(long_options[option_index].name,"injXMLnr")==0) {
	run->injXMLnr = atoi(optarg);
	printf("    - using injection %d from the injection XML file\n",run->injXMLnr);
      }
      break; //For case 0: long options
      
		case 'm':		
			run->parBestVal[0] = atof(optarg);
			run->commandflag[0] = 1;
			printf("\n    - From command line, Best Guess for mChirp\t\t= %f\n",run->parBestVal[0]);
			break;
		case 'e':		
			run->parBestVal[1] = atof(optarg);
			run->commandflag[1] = 1;
			printf("    - From command line, Best Guess for eta\t\t= %f\n",run->parBestVal[1]);
			break;
		case 't':		
			run->parBestVal[2] = atof(optarg);
			run->commandflag[2] = 1;
			printf("    - From command line, Best Guess for tc\t\t= %f\n",run->parBestVal[2]);
			break;
		case 'd':		
			run->parBestVal[3] = atof(optarg);
			run->commandflag[3] = 1;
			printf("    - From command line, Best Guess for log(distance)\t= %f\n",run->parBestVal[3]);
			break;
			
		case 'n':		
			run->nIter = atoi(optarg);
			run->commandsettingsflag[0] = 1;
			printf("\n    - From command line, number of iteration\t= %d\n",run->nIter);
			break;
		case 's':		
			run->thinOutput = atoi(optarg);
			run->commandsettingsflag[1] = 1;
			printf("    - From command line, thin output by\t= %d\n\n",run->thinOutput);
			break;

		case 'a': //Detector options
			if(strcmp(long_options[option_index].name,"nDet")==0) {
				run->networkSize = atoi(optarg);
				run->commandsettingsflag[2] = 1;
				printf("    - From command line, network size\t= %d\n",run->networkSize);
			}
		//	if(strcmp(long_options[option_index].name,"nDet")==0) {
		//		run->networkSize = atoi(optarg);
		//		run->commandsettingsflag[3] = 1;
		//		printf("    - From command line, network \t= %d\n",run->networkSize);
		//	}
			if(strcmp(long_options[option_index].name,"downsample")==0) {
				run->downsampleFactor = atoi(optarg);
				run->commandsettingsflag[4] = 1;
				printf("    - From command line, downsample factor\t= %d\n",run->downsampleFactor);
			}
			if(strcmp(long_options[option_index].name,"beforetc")==0) {
				run->dataBeforeTc = atof(optarg);
				run->commandsettingsflag[5] = 1;
				printf("    - From command line, before tc\t= %f\n",run->dataBeforeTc);
			}
			if(strcmp(long_options[option_index].name,"aftertc")==0) {
				run->dataAfterTc = atof(optarg);
				run->commandsettingsflag[6] = 1;
				printf("    - From command line, after tc\t= %f\n",run->dataAfterTc);
			}
			if(strcmp(long_options[option_index].name,"Flow")==0) {
				run->lowFrequencyCut = atof(optarg);
				run->commandsettingsflag[7] = 1;
				printf("    - From command line, low frequency cut\t= %f\n",run->lowFrequencyCut);
			}
			if(strcmp(long_options[option_index].name,"Fhigh")==0) {
				run->highFrequencyCut = atof(optarg);
				run->commandsettingsflag[8] = 1;
				printf("    - From command line, high frequency cut\t= %f\n",run->highFrequencyCut);
			}
			if(strcmp(long_options[option_index].name,"nPSDsegment")==0) {
				run->PSDsegmentNumber = atoi(optarg);
				run->commandsettingsflag[9] = 1;
				printf("    - From command line, number of PSD segments\t= %d\n",run->PSDsegmentNumber);
			}
			if(strcmp(long_options[option_index].name,"lPSDsegment")==0) {
				run->PSDsegmentLength = atof(optarg);
				run->commandsettingsflag[10] = 1;
				printf("    - From command line, length of PSD segments\t= %f\n",run->PSDsegmentLength);
			}
			break; 		
			
      
      // *** Treat the short and translated long options:
    case 'i':
      strcpy(run->mainFilename,optarg);
      printf("    - using main input file %s\n",run->mainFilename);
      break;
      
    default:
      //fprintf(stderr,"   Unrecognised option: %d\n",c);  // This notice is already produced by getopt_long()
      fprintf(stderr,USAGE); 
      exit(1);
      break;
      
    } // switch()
  } // while()
  
  
  // Print any remaining command line arguments (the ones that are not options, i.e. lack a starting '-'):
  if(optind < argc) {
    printf("   SPINspiral - unused command-line arguments: ");
    while(optind < argc) printf ("%s ", argv[optind++]);
    printf("\n");
  }
  
} // End void readCommandLineOptions(argc,argv)
// ****************************************************************************************************************************************************  







// ****************************************************************************************************************************************************  
/** 
 * \brief Read the main input file
 * 
 * All parameters that are read here should be(come) members of the runvar struct and lose their global status
 */
// ****************************************************************************************************************************************************  
void readMainInputfile(struct runPar *run)
{
  int i;
  char bla[500];
  FILE *fin;
  
  if((fin = fopen(run->mainFilename,"r")) == NULL) {
    fprintf(stderr, "\n\n   ERROR opening main input file: %s, aborting.\n\n\n",run->mainFilename);
    exit(1);
  } else {
    printf("   Using main input file: %s.\n",run->mainFilename);
  }
  
  
  //Use and l for floats: %lf, %lg, etc, rather than %f, %g
  
  for(i=1;i<=3;i++) fgets(bla,500,fin);  //Read first 3 lines
  
  //Operation and output:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->doSNR);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->doMCMC);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->doMatch);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->writeSignal);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->beVerbose);
  
  
  //Secondary input files:
  fgets(bla,500,fin); fgets(bla,500,fin); fgets(bla,500,fin); //Read the empty and comment lines
  fgets(bla,500,fin); sscanf(bla,"%s",run->mcmcFilename);
  fgets(bla,500,fin); sscanf(bla,"%s",run->dataFilename);
  fgets(bla,500,fin); sscanf(bla,"%s",run->injectionFilename);
  fgets(bla,500,fin); sscanf(bla,"%s",run->parameterFilename);
  fgets(bla,500,fin); sscanf(bla,"%s",run->systemFilename);
  
  fclose(fin);
}  //End of readMainInputfile
// ****************************************************************************************************************************************************  










// ****************************************************************************************************************************************************  
/** 
 * \brief Read the MCMC input file
 * 
 * All parameters that are read here should be(come) members of the runvar struct and lose their global status
 */
// ****************************************************************************************************************************************************  
void readMCMCinputfile(struct runPar *run)
{
  int i;
  double tmpdbl;
  char bla[500];
  FILE *fin;
  
  if((fin = fopen(run->mcmcFilename,"r")) == NULL) {
    fprintf(stderr, "\n\n   ERROR opening MCMC input file: %s, aborting.\n\n\n",run->mcmcFilename);
    exit(1);
  } else {
    printf("   Using MCMC input file: %s.\n",run->mcmcFilename);
  }
  
  
  //Use and l for floats: %lf, %lg, etc, rather than %f, %g
  
  for(i=1;i<=3;i++) { //Read first 3 lines
    fgets(bla,500,fin);
  }
  
  //Basic settings
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line

	  fgets(bla,500,fin);  
	if(run->commandsettingsflag[0] == 0) {
sscanf(bla,"%lg",&tmpdbl);
	run->nIter = (int)tmpdbl;
	}
  
	  fgets(bla,500,fin);
	if(run->commandsettingsflag[1] == 0) {	
  sscanf(bla,"%d",&run->thinOutput);
	}
		
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->thinScreenOutput);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->MCMCseed);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->adaptiveMCMC);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->acceptRateTarget);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->minlogL);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->blockFrac);
  
  

  //Correlated update proposals:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->correlatedUpdates);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->corrFrac);
  fgets(bla,500,fin);  sscanf(bla,"%lg",&tmpdbl);
  run->nCorr = (int)tmpdbl;
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->matAccFr);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->prMatrixInfo);
  
  
  //Annealing:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->annealTemp0);
  fgets(bla,500,fin);  sscanf(bla,"%lg",&tmpdbl);
  run->annealNburn = (int)tmpdbl;
  fgets(bla,500,fin);  sscanf(bla,"%lg",&tmpdbl);
  run->annealNburn0 = (int)tmpdbl;
  
  //Parallel tempering:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->parallelTempering);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->nTemps);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->maxTemp);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->saveHotChains);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->prParTempInfo);
  
  //Manual temperature ladder for parallel tempering:
  fgets(bla,500,fin); fgets(bla,500,fin); //Read the empty and comment line
  for(i=0;i<run->nTemps;i++) fscanf(fin,"%lf",&run->tempLadder[i]);  //Read the array directly, because sscanf cannot be in a loop...
  
  fclose(fin);
} //End of void readMCMCinputfile(struct runPar *run)
// ****************************************************************************************************************************************************  












// ****************************************************************************************************************************************************  
/** 
 * \brief Read the data input file
 */
// ****************************************************************************************************************************************************  
void readDataInputfile(struct runPar *run, struct interferometer ifo[])
{
  int i=0,j=0;
  double lati,longi,leftArm,rightArm;
  char bla[500], subdir[500];
  FILE *fin;
  
  if((fin = fopen(run->dataFilename,"r")) == NULL) {
    fprintf(stderr, "\n\n   ERROR opening data file: %s, aborting.\n\n\n",run->dataFilename);
    exit(1);
  } else {
    printf("   Using data input file: %s.\n",run->dataFilename);
  }
  
  
  //Use and l for floats: %lf, %lg, etc, rather than %f, %g
  
  for(j=1;j<=3;j++) fgets(bla,500,fin);  //Read first 3 lines
  fgets(run->datasetName,80,fin);  fgets(bla,500,fin);  //Read name of the data set used, and then the rest of the line
  
  //Detector network:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line

  fgets(bla,500,fin);
    if(run->commandsettingsflag[2] == 0) {	
    sscanf(bla,"%d",&run->networkSize);
	}
	
	
  for(i=0;i<run->networkSize;i++) fscanf(fin,"%d",&run->selectifos[i]);  //Read the array directly, because sscanf cannot be in a loop...
  fgets(bla,500,fin);  //Read the rest of the line
  
  
  //Data handling:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin); 
	if(run->commandsettingsflag[4] == 0) sscanf(bla,"%d",&run->downsampleFactor);
  fgets(bla,500,fin); 
	if(run->commandsettingsflag[5] == 0) sscanf(bla,"%lf",&run->dataBeforeTc);
  fgets(bla,500,fin); 
	if(run->commandsettingsflag[6] == 0) sscanf(bla,"%lf",&run->dataAfterTc);
  fgets(bla,500,fin); 
	if(run->commandsettingsflag[7] == 0) sscanf(bla,"%lf",&run->lowFrequencyCut);
  fgets(bla,500,fin); 
	if(run->commandsettingsflag[8] == 0) sscanf(bla,"%lf",&run->highFrequencyCut);
  fgets(bla,500,fin); sscanf(bla,"%lf",&run->tukeyWin);
  
  
  //Read input for PSD estimation:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment lines
  fgets(bla,500,fin);  
	if(run->commandsettingsflag[9] == 0) sscanf(bla,"%d",&run->PSDsegmentNumber);
  fgets(bla,500,fin);  
	if(run->commandsettingsflag[10] == 0) sscanf(bla,"%lf",&run->PSDsegmentLength);
  
  
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment lines
  for(i=0;i<run->networkSize;i++){
    fgets(bla,500,fin); fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment lines
    
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].name);
    fgets(bla,500,fin);  sscanf(bla,"%lf",&lati);
    fgets(bla,500,fin);  sscanf(bla,"%lf",&longi);
    fgets(bla,500,fin);  sscanf(bla,"%lf",&rightArm);
    fgets(bla,500,fin);  sscanf(bla,"%lf",&leftArm);
    
    ifo[i].lati      = lati     /180.0*pi;
    ifo[i].longi     = longi    /180.0*pi;
    ifo[i].rightArm  = rightArm /180.0*pi;
    ifo[i].leftArm   = leftArm  /180.0*pi;
    
    fgets(bla,500,fin);  //Read the empty line
    
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].ch1name);
    //fgets(bla,500,fin);  sscanf(bla,"%s",&ifo[i].ch1filepath);
    fgets(bla,500,fin);  sscanf(bla,"%s",subdir);
    sprintf(ifo[i].ch1filepath,"%s%s%s",run->dataDir,"/",subdir);
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].ch1fileprefix);
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].ch1filesuffix);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].ch1filesize);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].ch1fileoffset);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].ch1doubleprecision);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].add2channels);
    
    fgets(bla,500,fin);  //Read the empty line
    
    fgets(bla,500,fin);  sscanf(bla,"%ld",&ifo[i].noiseGPSstart);
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].noisechannel);
    //fgets(bla,500,fin);  sscanf(bla,"%s",&ifo[i].noisefilepath);
    fgets(bla,500,fin);  sscanf(bla,"%s",subdir);
    sprintf(ifo[i].noisefilepath,"%s%s%s",run->dataDir,"/",subdir);
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].noisefileprefix);
    fgets(bla,500,fin);  sscanf(bla,"%s",ifo[i].noisefilesuffix);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].noisefilesize);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].noisefileoffset);
    fgets(bla,500,fin);  sscanf(bla,"%d",&ifo[i].noisedoubleprecision);
    
  }
  fclose(fin);
  
}  //End of readDataInputfile
// ****************************************************************************************************************************************************  





// ****************************************************************************************************************************************************  
/** 
 * \brief Read the injection input file
 * 
 * All parameters that are read in here should be(come) members of the runvar struct
 */
// ****************************************************************************************************************************************************  
void readInjectionInputfile(struct runPar *run)
{
  int i;
  char bla[500];
  FILE *fin;
  
  // Open injection input file:
  if((fin = fopen(run->injectionFilename,"r")) == NULL) {
    fprintf(stderr, "\n\n   ERROR opening injection input file: %s, aborting.\n\n\n",run->injectionFilename);
    exit(1);
  } else {
    printf("   Using injection input file: %s.\n",run->injectionFilename);
  }
  
  
  // Read injection input file:
  // use and l for floats: %lf, %lg, etc, rather than %f, %g
  
  for(i=1;i<=2;i++) fgets(bla,500,fin);  //Read first 2 lines
  
  // Read general injection data:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->injectSignal);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->injectionWaveform);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->injectionPNorder);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->injectionSNR);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->injRanSeed);
  
  //Get the number of injection parameters from the injectionWaveform
  if(run->injectSignal >= 1) {
    if(run->injectionWaveform==1) {
      if(run->beVerbose>=1) printf("    - using Apostolatos, 1.5-pN, 12-parameter waveform for the software injection.\n"); // Only the 1.5-pN order is available
      run->nInjectPar=12;
    } else if(run->injectionWaveform==2) {
      if(run->beVerbose>=1) printf("    - using LAL,%4.1f-pN, 12-parameter waveform for the software injection.\n",run->injectionPNorder);
      run->nInjectPar=12;
    } else if(run->injectionWaveform==3) {
      if(run->beVerbose>=1) printf("    - using LAL,%4.1f-pN, 15-parameter waveform for the software injection.\n",run->injectionPNorder);
      run->nInjectPar=15;
    } else if(run->injectionWaveform==4) {
      if(run->beVerbose>=1) printf("    - using LAL,%4.1f-pN, non-spinning waveform for the software injection.\n",run->injectionPNorder);
      run->nInjectPar=9;
    } else {
      fprintf(stderr,"    - unknown waveform chosen as MCMC template: %d.   Available waveforms are:\n",run->injectionWaveform);
      fprintf(stderr,"        1: Apostolatos, simple precession, 12 parameters\n");
      fprintf(stderr,"        2: LAL, single spin, 12 parameters\n");
      fprintf(stderr,"        3: LAL, double spin, 15 parameters\n");
      fprintf(stderr,"        4: LAL, non-spinning, 9 parameters\n");
      fprintf(stderr,"      Please set injectionWaveform in %s to one of these values.\n\n",run->injectionFilename);
      exit(1);
    }
  }
  
  
  
  // Read injection parameters:
  for(i=1;i<=5;i++) fgets(bla,500,fin);  //Read empty and comment lines
  
  for(i=0;i<run->nInjectPar;i++) {
    fscanf(fin,"%d %d %lf %d %lf %d %lf %lf",&run->injNumber[i],&run->injID[i],&run->injParValOrig[i],&run->injRanPar[i],&run->injSigma[i],&run->injBoundType[i],&run->injBoundLow[i],&run->injBoundUp[i]);
    fgets(bla,500,fin);  //Read rest of the line
    
    //printf("%d %d %lf %d %lf %d %lf %lf\n",run->injNumber[i],run->injID[i],run->injParValOrig[i],run->injRanPar[i],run->injSigma[i],run->injBoundType[i],run->injBoundLow[i],run->injBoundUp[i]);
    
    
    // Some sanity checks:
    if(run->injNumber[i] != i+1) {
      fprintf(stderr, "\n\n   ERROR reading injection input file %s:  parameter %d has number %d.\n   Aborting...\n\n",run->injectionFilename,i+1,run->injNumber[i]);
      exit(1);
    }
    
    if(run->parDef[run->injID[i]] != 1) {
      fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d:\n     parameter ID %d is not defined.\n   Aborting...\n\n",
	      run->injectionFilename,run->injNumber[i],run->injID[i]);
      exit(1);
    }
    
    // Get reverse parameter ID:
    run->injRevID[run->injID[i]] = i;
    
    // Set this parameter as being used for the injection:
    run->injParUse[run->injID[i]] = 1;
    
    // Get the desired injection boundaries:
    switch (run->injBoundType[i]) {
    case 1 :  // Keep as is: boundLow-boundUp
      break; 
      
    case 2 :  // InjectionValue + BoundLow - InjectionValue + BoundUp
      if(run->injBoundLow[i] > 0.0 || run->injBoundUp[i] < 0.0) {
	fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d (%s):\n     for injBoundType = 2, injBoundLow and injBoundUp must be <= 0 and >= 0 respectively.\n   Aborting...\n\n",
	       run->injectionFilename,run->injNumber[i],run->parAbrev[run->injID[i]]);
	exit(1);
      }
      run->injBoundLow[i] = run->injParValOrig[i] + run->injBoundLow[i];
      run->injBoundUp[i]  = run->injParValOrig[i] + run->injBoundUp[i];
      break;
      
    case 3 :  // InjectionValue * BoundLow - InjectionValue * BoundUp
      if(run->injBoundLow[i] > 1.0 || run->injBoundUp[i] < 1.0) {
	fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d (%s):\n     for injBoundType = 3, injBoundLow and injBoundUp must be <= 1 and >= 1 respectively.\n   Aborting...\n\n",
		run->injectionFilename,run->injNumber[i],run->parAbrev[run->injID[i]]);
	exit(1);
      }
      run->injBoundLow[i] = run->injParValOrig[i] * run->injBoundLow[i];
      run->injBoundUp[i]  = run->injParValOrig[i] * run->injBoundUp[i];
      break;
      
    default :
      fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d (%s):\n     %d is not a valid option for injBoundType.\n   Aborting...\n\n",
	      run->injectionFilename,run->injNumber[i],run->parAbrev[run->injID[i]],run->injBoundType[i]);
      exit(1);
    } //End switch (run->injBoundType[i])
    
    
    
    // More sanity checks:
    
    // Check whether value for injRanPar is valid
    if(run->injRanPar[i] < 0 || run->injRanPar[i] > 2) {
      fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d (%s):\n     %d is not a valid option for injRanPar.\n   Aborting...\n\n",
	      run->injectionFilename,run->injNumber[i],run->parAbrev[run->injID[i]],run->injRanPar[i]);
      exit(1);
    }      
    
    //Check whether the lower boundary < the upper
    if(run->injBoundLow[i] >= run->injBoundUp[i]) {
      fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d (%s):\n     the lower boundary of the prior is larger than or equal to the upper boundary (%lf vs. %lf).\n   Aborting...\n\n",
	      run->injectionFilename,run->injNumber[i],run->parAbrev[run->injID[i]],run->injBoundLow[i],run->injBoundUp[i]);
      exit(1);
    }
    
    //Check whether  lower boundary <= injection value <= upper boundary
    if(run->injParValOrig[i] < run->injBoundLow[i] || run->injParValOrig[i] > run->injBoundUp[i]) {
      fprintf(stderr, "\n\n   ERROR reading injection input file %s, parameter %d (%s):\n     the injection value (%lf) lies outside the prior range (%lf - %lf).\n   Aborting...\n\n",
	      run->injectionFilename,run->injNumber[i],run->parAbrev[run->injID[i]], run->injParValOrig[i], run->injBoundLow[i], run->injBoundUp[i]);
      exit(1);
    }
    
  } //End for (i): injection parameter
  
  
  // Pick random injection values:
  setRandomInjectionParameters(run);    //Copy the injection parameters from injParValOrig to injParVal, and randomise where wanted
  run->geocentricTc = run->injParVal[run->injRevID[11]];    // This value must be overwritten by the 'best' value in readParameterInputfile() which is called next, in the case of no SW injection
  
  fclose(fin);
  
  
  
  // Print injection parameters and prior ranges to screen:
  if(run->beVerbose>=1) {
    if(run->injectSignal >= 1) {
      printf("\n      %2i software-injection parameters:\n        Nr:  ID: Name:           Injection value:     Obtained:\n",run->nInjectPar);
      for(i=0;i<run->nInjectPar;i++) {
	if(run->injRanPar[i]==0) {
	  printf("        %2d  %3i  %-11s     %15.4lf      Taken from the value set in %s\n",i,run->injID[i],run->parAbrev[run->injID[i]],run->injParVal[i],
		 run->injectionFilename);
	} else if(run->injRanPar[i]==1) {
	  printf("        %2d  %3i  %-11s     %15.4lf      Drawn randomly from a Gaussian distribution with centre  %lf  and width  %lf\n",i,run->injID[i],
		 run->parAbrev[run->injID[i]],run->injParVal[i],run->injParValOrig[i],run->injSigma[i]);
	} else if(run->injRanPar[i]==2) {
	  printf("        %2d  %3i  %-11s     %15.4lf      Drawn randomly from a uniform distribution  %14.4lf - %-14.4lf\n",i,run->injID[i],run->parAbrev[run->injID[i]]
		 ,run->injParVal[i],run->injBoundLow[i],run->injBoundUp[i]);
	}
      }
      printf("\n");
    } else {
      printf("    - not injecting a signal\n");
    }
  }
  
  
}  //End of readInjectionInputfile
// ****************************************************************************************************************************************************  













// ****************************************************************************************************************************************************  
/** 
 * \brief Read the (MCMC) parameter input file
 * 
 * All parameters that are read in here should be(come) members of the runvar struct
 */
// ****************************************************************************************************************************************************  
void readParameterInputfile(struct runPar *run)
{
  int i,iInj;
  char bla[500];
  FILE *fin;
	double dump = 0.0;
	
  if((fin = fopen(run->parameterFilename,"r")) == NULL) {
    fprintf(stderr, "\n\n   ERROR opening parameter input file: %s, aborting.\n\n\n",run->parameterFilename);
    exit(1);
  } else {
    printf("   Using parameter input file: %s.\n",run->parameterFilename);
  }
  
  
  //Use and l for floats: %lf, %lg, etc, rather than %f, %g
  
  for(i=1;i<=2;i++) fgets(bla,500,fin);  //Read first 2 lines
  
  //Priors:
  fgets(bla,500,fin); fgets(bla,500,fin);  //Read the empty and comment line
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->mcmcWaveform);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->mcmcPNorder);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->priorSet);
  fgets(bla,500,fin);  sscanf(bla,"%d",&run->offsetMCMC);
  fgets(bla,500,fin);  sscanf(bla,"%lf",&run->offsetX);
  
  if(run->mcmcWaveform==1) {
    if(run->beVerbose>=1) printf("    - using Apostolatos, 1.5-pN, 12-parameter waveform as the MCMC template.\n");  //Only 1.5-pN order is available
    run->nMCMCpar=12;
  } else if(run->mcmcWaveform==2) {
    if(run->beVerbose>=1) printf("    - using LAL,%4.1f-pN, 12-parameter waveform as the MCMC template.\n",run->mcmcPNorder);
    run->nMCMCpar=12;
  } else if(run->mcmcWaveform==3) {
    if(run->beVerbose>=1) printf("    - using LAL,%4.1f-pN, 15-parameter waveform as the MCMC template.\n",run->mcmcPNorder);
    run->nMCMCpar=15;
  } else if(run->mcmcWaveform==4) {
    if(run->beVerbose>=1) printf("    - using LAL,%4.1f-pN, non-spinning waveform as the MCMC template.\n",run->mcmcPNorder);
    run->nMCMCpar=9;
  } else {
    fprintf(stderr,"    - unknown waveform chosen as MCMC template: %d.   Available waveforms are:\n",run->mcmcWaveform);
    fprintf(stderr,"        1: Apostolatos, simple precession, 12 parameters\n");
    fprintf(stderr,"        2: LAL, single spin, 12 parameters\n");
    fprintf(stderr,"        3: LAL, double spin, 15 parameters\n");
    fprintf(stderr,"        4: LAL, non-spinnig, 9 parameters\n");
    fprintf(stderr,"      Please set mcmcWaveform in %s to one of these values.\n\n",run->mcmcFilename);
    exit(1);
  }
  
  
  
  
  //Parameters:
  for(i=1;i<=5;i++) fgets(bla,500,fin);  //Read empty and comment lines
  
  int warnings = 0;
  for(i=0;i<run->nMCMCpar;i++) {
	if(run->commandflag[i] == 0) {
    fscanf(fin,"%d %d %lf %d %d %lf %d %lf %lf",&run->parNumber[i],&run->parID[i],&run->parBestVal[i],&run->parFix[i],&run->parStartMCMC[i],&run->parSigma[i],&run->priorType[i],&run->priorBoundLow[i],&run->priorBoundUp[i]);
    }
	else {	
	fscanf(fin,"%d %d %lf %d %d %lf %d %lf %lf",&run->parNumber[i],&run->parID[i],&dump,&run->parFix[i],&run->parStartMCMC[i],&run->parSigma[i],&run->priorType[i],&run->priorBoundLow[i],&run->priorBoundUp[i]);	
	}	
	fgets(bla,500,fin);  //Read rest of the line
    
    //printf("%d:  %d %d %lf %d %lf %d %lf %lf\n",i,run->parNumber[i],run->parID[i],run->parBestVal[i],run->parStartMCMC[i],run->parSigma[i],
    //   run->priorType[i],run->priorBoundLow[i],run->priorBoundUp[i]);
    
    
    if(run->parNumber[i] != i+1) {
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s:  parameter %d has number %d.\n   Aborting...\n\n",run->parameterFilename,i+1,run->parNumber[i]);
      exit(1);
    }
    
    if(run->parDef[run->parID[i]] != 1) {
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d:\n     parameter ID %d is not defined.\n   Aborting...\n\n",
	     run->injectionFilename,run->parNumber[i],run->parID[i]);
      exit(1);
    }
	  

    
    // Set the reverse parameter ID:
    run->parRevID[run->parID[i]] = i;
        
    // Set this parameter as being used for MCMC:
    run->mcmcParUse[run->parID[i]] = 1;

    
    
    // Get the desired boundary conditions:
    switch (run->priorType[i]) {
    case 11 :
      break;
    case 12 :
      if(run->priorBoundLow[i] > 0.0 || run->priorBoundUp[i] < 0.0) {
	fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     for priorType = 12, priorBoundLow and priorBoundUp must be <= 0 and >= 0 respectively.\n   Aborting...\n\n",
	       run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]]);
	exit(1);
      }
      run->priorBoundLow[i] = run->parBestVal[i] + run->priorBoundLow[i];
      run->priorBoundUp[i]  = run->parBestVal[i] + run->priorBoundUp[i];
      break;
    case 13 :
      if(run->priorBoundLow[i] > 1.0 || run->priorBoundUp[i] < 1.0) {
	fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     for priorType = 13, priorBoundLow and priorBoundUp must be <= 1 and >= 1 respectively.\n   Aborting...\n\n",
	       run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]]);
	exit(1);
      }
      run->priorBoundLow[i] = run->parBestVal[i] * run->priorBoundLow[i];
      run->priorBoundUp[i]  = run->parBestVal[i] * run->priorBoundUp[i];
      break;
    case 21 : 
      run->priorBoundLow[i] = 0.0;
      run->priorBoundUp[i]  = tpi;
      break;
    case 22 :
      run->priorBoundLow[i] = 0.0;
      run->priorBoundUp[i]  = pi;
      break;
    default :
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     %d is not a valid option for priorType.\n   Aborting...\n\n",
	     run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]],run->priorType[i]);
      exit(1);
    } //End switch
    
    
    // Check whether value for fix is valid
    if(run->parFix[i] < 0 || run->parFix[i] > 2) {
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     %d is not a valid option for parFix.\n   Aborting...\n\n",
	     run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]],run->parFix[i]);
	exit(1);
    }      
    
    // Check whether value for start is valid
    if(run->parStartMCMC[i] < 1 || run->parStartMCMC[i] > 5) {
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     %d is not a valid option for parStartMCMC.\n   Aborting...\n\n",
	      run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]],run->parStartMCMC[i]);
      exit(1);
    }
    if((run->parStartMCMC[i] == 3 || run->parStartMCMC[i] == 4) && run->injectSignal <= 0) {
      fprintf(stdout, "    - no software injection was performed, so I'll use bestValue rather than injectionValue for %s.\n",run->parAbrev[run->parID[i]]);
      run->parStartMCMC[i] -= 2;
    }

    
    //Check whether the lower prior boundary < the upper
    if(run->priorBoundLow[i] >= run->priorBoundUp[i]) {
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     the lower boundary of the prior is larger than or equal to the upper boundary (%lf vs. %lf).\n   Aborting...\n\n",
	     run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]],run->priorBoundLow[i],run->priorBoundUp[i]);
      exit(1);
    }
    
    //Check whether  lower prior boundary <= best value <= upper boundary
    if(run->parBestVal[i] < run->priorBoundLow[i] || run->parBestVal[i] > run->priorBoundUp[i]) {
      fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     the best value (%lf) lies outside the prior range (%lf - %lf).\n   Aborting...\n\n",
	     run->parameterFilename,run->parNumber[i],run->parAbrev[run->parID[i]], run->parBestVal[i], run->priorBoundLow[i], run->priorBoundUp[i]);
      exit(1);
    }
    
    //Check whether  lower prior boundary <= INJECTION value <= upper boundary
    iInj = run->injRevID[run->parID[i]];  //Get the index of this parameter in the injection set.  -1 if not available.
    if(iInj >= 0) {
      if(run->injParVal[iInj] < run->priorBoundLow[i] || run->injParVal[iInj] > run->priorBoundUp[i]) {
	fprintf(stderr, "\n\n   ERROR reading parameter input file %s, parameter %d (%s):\n     the injection value (%lf) lies outside the prior range (%lf - %lf).\n   Aborting...\n\n",
	       run->parameterFilename, run->parNumber[i], run->parAbrev[run->parID[i]], run->injParVal[iInj], run->priorBoundLow[i], run->priorBoundUp[i]);
	exit(1);
      }
    } else {
      if(run->injectSignal != 0) {
	if(warnings==0) fprintf(stderr, "\n");
	printf("    * Warning:  MCMC parameter %i (%s) does not occur in the injection template;  I cannot verify whether the injection value lies within the prior range *\n",
		run->parNumber[i],run->parAbrev[run->parID[i]]);
	warnings += 1;
      }
    } 
  } //End for (i)
  //if(warnings >0) printf("\n");
  
  
  if(run->injectSignal<=0) {
    run->geocentricTc = run->parBestVal[run->parRevID[11]];    // This value overwrites the injection value from readInjectionInputfile(), in the case of no SW injection
    for(i=0;i<run->nMCMCpar;i++) run->injParVal[i] = run->parBestVal[i];   //CHECK Needed to avoid SegFault in the case of t_c
  }

  fclose(fin);
  
  
  
  //Print MCMC parameters and prior ranges to screen:
  char FixStr[3][99];
  strcpy(FixStr[0],"No, let it free");
  strcpy(FixStr[1],"Yes, to best value");
  strcpy(FixStr[2],"Yes, to injection");
  
  char StartStr[6][99];
  strcpy(StartStr[1],"From the best value");
  strcpy(StartStr[2],"Randomly from a Gaussian around the best value");
  strcpy(StartStr[3],"From the injection value");
  strcpy(StartStr[4],"Randomly from a Gaussian around the injection");
  strcpy(StartStr[5],"Randomly from the prior");
  
  
  //Write parameter choice to screen:
  if(run->beVerbose>=1) {
    printf("\n      %2i MCMC parameters:\n        Nr:  ID: Name:                Best value:     Prior:     min:            max:    Fix parameter?        Start chain:\n",run->nMCMCpar);
    for(i=0;i<run->nMCMCpar;i++) {
      printf("        %2d  %3i  %-11s     %15.4lf     %15.4lf %15.4lf     %-20s  %-45s\n",i,run->parID[i],run->parAbrev[run->parID[i]],run->parBestVal[i],
	     run->priorBoundLow[i],run->priorBoundUp[i],  FixStr[run->parFix[i]],StartStr[run->parStartMCMC[i]]);
    }
    printf("\n");
  }
  
}  //End of readParameterInputfile
// ****************************************************************************************************************************************************  








// ****************************************************************************************************************************************************  
/** 
 * \brief Read the input file for system (system-dependent) variables, e.g. SPINspiral.input.system
 */
// ****************************************************************************************************************************************************  
void readSystemInputfile(struct runPar *run)
{
  int i;
  char bla[500];
  FILE *fin;
  
  if((fin = fopen(run->systemFilename,"r")) == NULL) {
    fprintf(stderr, "\n\n   ERROR opening system file: %s, aborting.\n\n\n",run->systemFilename);
    exit(1);
  } else {
    printf("   Using system input file: %s.\n",run->parameterFilename);
  }
  
  //Use and l for floats: %lf, %lg, etc, rather than %f, %g
  for(i=1;i<=3;i++) { //Read first 3 lines
    fgets(bla,500,fin);
  }  

  //Data directory:
  fscanf(fin, "%s",run->dataDir);
  
  fclose(fin);
}  //End of readSystemInputfile
// ****************************************************************************************************************************************************  





// ****************************************************************************************************************************************************  
/** 
 * \brief Read an XML injection file if specified
 */
// ****************************************************************************************************************************************************  
void readInjectionXML(struct runPar *run)
{
  int NumInj=0,i=0,j=0;
  SimInspiralTable *injTable = NULL;
  double Sx=0.0,Sy=0.0,Sz=0.0,Sxy=0.0,S=0.0;
  
  printf("  Reading injection XML file %s, injection %d.\n",run->injXMLfilename,run->injXMLnr);
  NumInj = SimInspiralTableFromLIGOLw(&injTable,run->injXMLfilename,0,0);
  if(NumInj <= 0) {
    fprintf(stderr,"\n\n   ERROR opening XML file %s.\n   Aborting.\n\n",run->injXMLfilename);
    exit(1);
  }
  if(run->injXMLnr >= NumInj) {
    fprintf(stderr,"\n\n  ERROR: requested injection number %d larger than number of injections (%d) in XML file %s.\n  Aborting.\n\n",run->injXMLnr,NumInj-1,run->injXMLfilename);
    exit(1);
  }
  
  j=0;
  while(j<run->injXMLnr) {j++; injTable = injTable->next;}  // Select injection

  i=0;
  for(i=0;i<run->nInjectPar;i++) {
    
    //Time:
    if(run->injID[i] == 11) run->injParVal[i] = injTable->geocent_end_time.gpsSeconds+injTable->geocent_end_time.gpsNanoSeconds*1.0e-9;  // t_c in geocentre
    
    
    //Distance:
    if(run->injID[i] == 21) run->injParVal[i] = pow(injTable->distance,3.0);  // d_L^3
    if(run->injID[i] == 22) run->injParVal[i] = log(injTable->distance);      // log(d_L/Mpc)
    
    
    //Sky location:
    if(run->injID[i] == 31) run->injParVal[i] = injTable->longitude;          // RA
    if(run->injID[i] == 32) run->injParVal[i] = sin(injTable->latitude);      // sin Dec
    
    
    //Orbital/GW phase:
    if(run->injID[i] == 41) run->injParVal[i] = injTable->coa_phase;          // \phi_c
    
    
    //Binary orientation:
    if(run->injID[i] == 51) run->injParVal[i] = cos(injTable->inclination);   // cos(i)
    if(run->injID[i] == 52) run->injParVal[i] = injTable->polarization;       // \psi
    if(run->injID[i] == 53) {
      printf("\n    * You're reading injection data from an XML file, while using the Apostolatos J_0.  This may lead to unexpected and unwanted results *\n");
      run->injParVal[i] = sin(injTable->inclination);   // sin(\theta_J0), should probably not be used
    }
    if(run->injID[i] == 54) {
      printf("\n    * You're reading injection data from an XML file, while using the Apostolatos J_0.  This may lead to unexpected and unwanted results *\n");
      run->injParVal[i] = injTable->polarization;       // \phi_J0, should probably not be used
    }
    
    
    //Mass:
    if(run->injID[i] == 61) run->injParVal[i] = injTable->mchirp;             // Mc
    if(run->injID[i] == 62) run->injParVal[i] = injTable->eta;                // \eta
    if(run->injID[i] == 63) run->injParVal[i] = injTable->mass1;              // M1
    if(run->injID[i] == 64) run->injParVal[i] = injTable->mass2;              // M2
    
    
    //Spin 1:
    Sx = injTable->spin1x;
    Sy = injTable->spin1y;
    Sz = injTable->spin1z;
    Sxy = sqrt(Sx*Sx+Sy*Sy);
    S = sqrt(Sxy*Sxy+Sz*Sz);
    if(run->injID[i] == 71) run->injParVal[i] = S;                            // Spin1 magnitude (a_1)
    if(run->injID[i] == 72) {
      run->injParVal[i] = Sz/S;                                               // cos(\theta_1): angle between S1 and L (at t_c(?)))
      if(fabs(S)<1.0e-9) run->injParVal[i] = 1.0;                             // In case S1=0
    }
    if(run->injID[i] == 73) run->injParVal[i] = atan2(Sy,Sx);                 // \phi_1: phase of spin in orbital plane (at t_c(?))
    
    if(run->injID[i] == 75) run->injParVal[i] = Sx;                           // Spin1_x magnitude
    if(run->injID[i] == 76) run->injParVal[i] = Sy;                           // Spin1_y magnitude
    if(run->injID[i] == 77) run->injParVal[i] = Sz;                           // Spin1_z magnitude
    
    
    //Spin 2:
    Sx = injTable->spin2x;
    Sy = injTable->spin2y;
    Sz = injTable->spin2z;
    Sxy = sqrt(Sx*Sx+Sy*Sy);
    S = sqrt(Sxy*Sxy+Sz*Sz);
    if(run->injID[i] == 81) run->injParVal[i] = S;                            // Spin2 magnitude (a_2)
    if(run->injID[i] == 82) {
      run->injParVal[i] = Sz/S;                                               // cos(\theta_2): angle between S2 and L (at t_c(?)))
      if(fabs(S)<1.0e-9) run->injParVal[i] = 1.0;                             // In case S2=0
    }
    if(run->injID[i] == 83) run->injParVal[i] = atan2(Sy,Sx);                 // \phi_2: phase of spin in orbital plane (at t_c(?))
    
    if(run->injID[i] == 85) run->injParVal[i] = Sx;                           // Spin2_x magnitude
    if(run->injID[i] == 86) run->injParVal[i] = Sy;                           // Spin2_y magnitude
    if(run->injID[i] == 87) run->injParVal[i] = Sz;                           // Spin2_z magnitude
    
    
    //Merger, ringdown, ...:
    //if(run->injID[i] == 91) run->injParVal[i] = injTable->;                   // 
    
    printf("    - %2d  %-11s     %15.4lf\n",run->injNumber[i],run->parAbrev[run->injID[i]],run->injParVal[i]);
    
  } // i (injectPar)
  
  
  run->lowFrequencyCutInj = injTable->f_lower;  // May be 0.0!

  printf("\n");
  
} // End void readInjectionXML()
// ****************************************************************************************************************************************************  











// ****************************************************************************************************************************************************  
/** 
 * \brief Get random values for the injection parameters.
 */
// ****************************************************************************************************************************************************  
void setRandomInjectionParameters(struct runPar *run)  
{
  int i=0;
  gsl_rng *ran;
  double ranGauss = 0.0, ranUnif=0.0, db=0.0;
  ran = gsl_rng_alloc(gsl_rng_mt19937);  // GSL random-number seed
  
  // Manually select a random seed, *** USE ONLY FOR TESTING ***
  if(1==2 && run->injRanSeed == 0) {
    printf("\n  *** SELECTING RANDOM SEED ***  This should only be done while testing!!! setRandomInjectionParameters() \n\n");
    run->injRanSeed = 0;
    setSeed(&run->injRanSeed);
    printf("   Seed: %d\n", run->injRanSeed);
  }
  
  gsl_rng_set(ran, run->injRanSeed);     // Set seed
  
  for(i=0;i<run->nInjectPar;i++) {
    ranGauss = gsl_ran_gaussian(ran,run->injSigma[i]);                                    //Make sure you always draw the same number of random variables
    ranUnif = gsl_rng_uniform(ran);                                                       //Make sure you always draw the same number of random variables
    if(run->injRanPar[i]==0) {                  
      run->injParVal[i] = run->injParValOrig[i];                                          //Keep the suggested value
    } else if(run->injRanPar[i]==1) {                                                     
      run->injParVal[i] = run->injParValOrig[i] + ranGauss;                               //Draw random number from Gaussian with width ranGauss
      run->injParVal[i] = max(run->injParVal[i],run->injBoundLow[i]);                     //Stick to the boundary, rather than redrawing to keep number of random numbers constant
      run->injParVal[i] = min(run->injParVal[i],run->injBoundUp[i]);
    } else if(run->injRanPar[i]==2) {
      db = run->injBoundUp[i]-run->injBoundLow[i];                                        //Width of the range
      run->injParVal[i] = run->injBoundLow[i] + ranUnif*db;                               //Draw random number from uniform range with width db
    }
  }
  
  gsl_rng_free(ran);
} // End setRandomInjectionParameters()
// ****************************************************************************************************************************************************  





// ****************************************************************************************************************************************************  
/** 
 * \brief Set parameters names in the hardcoded parameter database
 */
// ****************************************************************************************************************************************************  
void setParameterNames(struct runPar * run)
{
  
  //Set 01: time
  strcpy(run->parAbrev[11], "t_c");
  strcpy(run->parAbrv[11], "t_c");
  run->parDef[11] = 1;
  strcpy(run->parAbrev[12], "t_40");
  strcpy(run->parAbrv[12], "t_40");
  run->parDef[12] = 1;
  
  //Set 02: distance
  strcpy(run->parAbrev[21], "d^3");
  strcpy(run->parAbrv[21], "d^3");
  run->parDef[21] = 1;
  strcpy(run->parAbrev[22], "log(d)");
  strcpy(run->parAbrv[22], "logD");
  run->parDef[22] = 1;
  
  //Set 03: sky position
  strcpy(run->parAbrev[31], "R.A.");
  strcpy(run->parAbrv[31], "RA");
  run->parDef[31] = 1;
  strcpy(run->parAbrev[32], "sin(dec)");
  strcpy(run->parAbrv[32], "sdec");
  run->parDef[32] = 1;
  
  //Set 04: phase
  strcpy(run->parAbrev[41], "phi_orb");
  strcpy(run->parAbrv[41], "phio");
  run->parDef[41] = 1;
  
  //Set 05: orientation
  strcpy(run->parAbrev[51], "cos(i)");
  strcpy(run->parAbrv[51], "cosi");
  run->parDef[51] = 1;
  strcpy(run->parAbrev[52], "psi");
  strcpy(run->parAbrv[52], "psi");
  run->parDef[52] = 1;
  strcpy(run->parAbrev[53], "sin th_J0");
  strcpy(run->parAbrv[53], "thJ0");
  run->parDef[53] = 1;
  strcpy(run->parAbrev[54], "phi_J0");
  strcpy(run->parAbrv[54], "phJ0");
  run->parDef[54] = 1;
  
  //Set 06: mass
  strcpy(run->parAbrev[61], "Mc");
  strcpy(run->parAbrv[61], "Mc");
  run->parDef[61] = 1;
  strcpy(run->parAbrev[62], "eta");
  strcpy(run->parAbrv[62], "eta");
  run->parDef[62] = 1;
  strcpy(run->parAbrev[63], "M1");
  strcpy(run->parAbrv[63], "M1");
  run->parDef[63] = 1;
  strcpy(run->parAbrev[64], "M2");
  strcpy(run->parAbrv[64], "M2");
  run->parDef[64] = 1;
  
  //Set 07: spin1
  strcpy(run->parAbrev[71], "a_spin1");
  strcpy(run->parAbrv[71], "asp1");
  run->parDef[71] = 1;
  strcpy(run->parAbrev[72], "cs th_sp1");
  strcpy(run->parAbrv[72], "ths1");
  run->parDef[72] = 1;
  strcpy(run->parAbrev[73], "phi_spin1");
  strcpy(run->parAbrv[73], "phs1");
  run->parDef[73] = 1;
  
  strcpy(run->parAbrev[75], "S1_x");
  strcpy(run->parAbrv[75], "S1x");
  run->parDef[75] = 1;
  strcpy(run->parAbrev[76], "S1_y");
  strcpy(run->parAbrv[76], "s1y");
  run->parDef[76] = 1;
  strcpy(run->parAbrev[77], "S1_z");
  strcpy(run->parAbrv[77], "S1z");
  run->parDef[77] = 1;
  
  //Set 08: spin2
  strcpy(run->parAbrev[81], "a_spin2");
  strcpy(run->parAbrv[81], "asp2");
  run->parDef[81] = 1;
  strcpy(run->parAbrev[82], "cs th_sp2");
  strcpy(run->parAbrv[82], "ths2");
  run->parDef[82] = 1;
  strcpy(run->parAbrev[83], "phi_spin2");
  strcpy(run->parAbrv[83], "phs2");
  run->parDef[83] = 1;

  strcpy(run->parAbrev[85], "S2_x");
  strcpy(run->parAbrv[85], "S2x");
  run->parDef[85] = 1;
  strcpy(run->parAbrev[86], "S2_y");
  strcpy(run->parAbrv[86], "s2y");
  run->parDef[86] = 1;
  strcpy(run->parAbrev[87], "S2_z");
  strcpy(run->parAbrv[87], "S2z");
  run->parDef[87] = 1;
  
  
  //Set 09: merger, ringdown
  //strcpy(run->parAbrev[], "");
  //run->parDef[] = 1;
  
  //Set:
  //strcpy(run->parAbrev[], "");
  //run->parDef[] = 1;
  
} // End setParameterNames()
// ****************************************************************************************************************************************************  













// ****************************************************************************************************************************************************  
/**
 * \brief Copy some of the elements of the struct runPar to the struct MCMCvariables
 */
// ****************************************************************************************************************************************************  
void copyRun2MCMC(struct runPar run, struct MCMCvariables *mcmc)
{
  int i=0,j=0;
  
  mcmc->beVerbose = run.beVerbose;                      // Print long stretches of output
  
  mcmc->maxnPar = run.maxnPar;                          // Absolute maximum number of mcmc/template parameters allowed
  mcmc->parDBn = run.parDBn;                            // Number of elements in hardcoded parameter database
  mcmc->nMCMCpar = run.nMCMCpar;                        // Number of mcmc/template parameters
  mcmc->nInjectPar = run.nInjectPar;                    // Number of mcmc/template parameters
  
  mcmc->mcmcWaveform = run.mcmcWaveform;                // Waveform used as MCMC template
  mcmc->mcmcPNorder = run.mcmcPNorder;                  // pN order used for MCMC template
  mcmc->injectSignal = run.injectSignal;                // Inject a signal in the data or not
  mcmc->injectionWaveform = run.injectionWaveform;      // Waveform used as injection template
  mcmc->injectionPNorder = run.injectionPNorder;        // pN order used for injection template
  mcmc->networkSize = run.networkSize;                  // Network size
  mcmc->seed = run.MCMCseed;                            // MCMC seed
  mcmc->blockFrac = run.blockFrac;                      // Fraction of non-correlated updates that is a block update
  mcmc->corrFrac = run.corrFrac;                        // Fraction of MCMC updates that used the correlation matrix
  mcmc->matAccFr = run.matAccFr;                        // Fraction of elements on the diagonal that must 'improve' in order to accept a new covariance matrix
  mcmc->offsetMCMC = run.offsetMCMC;                    // Start MCMC offset (i.e., not from injection values) or not
  mcmc->offsetX = run.offsetX;                          // Start offset chains from a Gaussian distribution offsetX times wider than parSigma
  
  mcmc->nIter = run.nIter;                              // Number of MCMC iterations to compute
  mcmc->thinOutput = run.thinOutput;                    // Save every thiOutput-th MCMC iteration to file
  mcmc->thinScreenOutput = run.thinScreenOutput;        // Save every thiOutput-th MCMC iteration to screen
  mcmc->adaptiveMCMC = run.adaptiveMCMC;                // Use adaptive MCMC
  mcmc->acceptRateTarget = run.acceptRateTarget;        // Target acceptance rate for MCMC
  mcmc->minlogL = run.minlogL;                          // Minimum value for the log Likelihood to accept a jump. We used 0 for a long time, this number shouldn't be positive!
  
  
  mcmc->correlatedUpdates = run.correlatedUpdates;      // Switch to do correlated update proposals
  mcmc->nCorr = run.nCorr;                              // Number of iterations for which the covariance matrix is calculated
  mcmc->prMatrixInfo = run.prMatrixInfo;                // Print information to screen on proposed matrix updates
  
  mcmc->annealTemp0 = run.annealTemp0;                  // Starting temperature of the annealed chain
  mcmc->annealNburn = run.annealNburn;                  // Number of iterations for the annealing burn-in phase
  mcmc->annealNburn0 = run.annealNburn0;                // Number of iterations during which temp=annealTemp0
  
  mcmc->parallelTempering = run.parallelTempering;      // Switch to use parallel tempering
  mcmc->maxTemp = run.maxTemp;                          // Maximum temperature in automatic parallel-tempering ladder
  mcmc->saveHotChains = run.saveHotChains;              // Save hot (T>1) parallel-tempering chains
  mcmc->prParTempInfo = run.prParTempInfo;              // Print information on the temperature chains
  
  
  mcmc->chTemp = max(mcmc->annealTemp0,1.0);            // Current temperature
  
  mcmc->geocentricTc = run.geocentricTc;                // Geocentric Tc
  mcmc->baseTime = (double)((floor)(mcmc->geocentricTc/100.0)*100);  //'Base' time, gets rid of the first 6-7 digits of GPS time
  
  
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
    mcmc->mcmcParUse[i] = run.mcmcParUse[i];
    mcmc->injParUse[i]  = run.injParUse[i];
    
    
    for(j=0;j<99;j++) {
      mcmc->parName[i][j] = run.parName[i][j];
      mcmc->parAbrev[i][j] = run.parAbrev[i][j];
      mcmc->parAbrv[i][j] = run.parAbrv[i][j];
    }
  }
  
  mcmc->nTemps = run.nTemps;                            // Size of temperature ladder
  for(i=0;i<mcmc->nTemps;i++) mcmc->tempLadder[i] = run.tempLadder[i];

} // End copyRun2MCMC()
// ****************************************************************************************************************************************************  





  






// ****************************************************************************************************************************************************  
/** 
 * \brief Set global variables
 *
 * \todo This routine should eventually contain mathematical and (astro)physical constants only.
 */
// ****************************************************************************************************************************************************  
void setConstants()
{
  // Mathematical constants:
  pi   = 3.141592653589793;   // pi
  tpi  = 6.283185307179586;   // 2 pi
  mtpi = 6.283185307179586e6; // Large multiple of 2 pi (2 megapi)
  
  // Define some physical constants:
  G    = 6.67259e-11;         // 6.674215e-11; */ /* gravity constant (SI)
  c    = 299792458.0;         // speed of light (m/s)
  
  Ms   = 1.9889194662e30;     // solar mass (kg)
  Mpc  = 3.08568025e22;       // metres in a Mpc  (LAL: 3.0856775807e22)
  Mpcs = 1.029272137e14;      // seconds in a Mpc  (Mpc/c)
} // End setConstants
// ****************************************************************************************************************************************************  








// ****************************************************************************************************************************************************  
/** 
 * \brief Returns the 'injection values' to the parameter set par
 *
 * \todo Remove par->mc etc. struct elements
 */
// ****************************************************************************************************************************************************  
void getInjectionParameters(struct parSet *par, int nInjectionPar, double *injParVal)
{
  int i=0;
  for(i=0;i<nInjectionPar;i++) {
    par->par[i] = injParVal[i];
  }
  par->nPar = nInjectionPar;
  
} // End getInjectionParameters()
// ****************************************************************************************************************************************************  




// ****************************************************************************************************************************************************  
/** 
 * \brief Returns the 'best-guess values' to the parameter set par
 */
// ****************************************************************************************************************************************************  
void getStartParameters(struct parSet *par, struct runPar run)  //Set the parameters to the starting values for the MCMC chain
{
  int i=0;
  for(i=0;i<run.nMCMCpar;i++) {
    par->par[i]      = run.parBestVal[i];
  }
  par->nPar = run.nMCMCpar;
  
} // End getStartParameters
// ****************************************************************************************************************************************************  



// ****************************************************************************************************************************************************  
/** 
 * \brief Allocate memory for the vectors in the struct parSet
 */
// ****************************************************************************************************************************************************  
void allocParset(struct parSet *par, int networkSize)
{
  par->loctc    = NULL;
  par->localti  = NULL;
  par->locazi   = NULL;
  
  par->loctc    = (double*)calloc(networkSize,sizeof(double));
  par->localti  = (double*)calloc(networkSize,sizeof(double));
  par->locazi   = (double*)calloc(networkSize,sizeof(double));
} // End allocParset
// ****************************************************************************************************************************************************  




// ****************************************************************************************************************************************************  
/** 
 * \brief Deallocate memory for the vectors in the struct parSet
 */
// ****************************************************************************************************************************************************  
void freeParset(struct parSet *par)
{
  free(par->loctc);         par->loctc        = NULL;
  free(par->localti);       par->localti      = NULL;
  free(par->locazi);        par->locazi       = NULL;
} // End freeParset
// ****************************************************************************************************************************************************  







