// Dear emacs, this is -*- c++ -*-
#ifndef PHYS_GENOLIKE_CALC_H
#define PHYS_GENOLIKE_CALC_H

//Angsd includes
#include "bambi_interface.h"

// C++ includes
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

extern int refToInt[256];
class phys_genolike_calc {

 private : 
  
  // Information stored in chunkyT
  chunkyT *chk;
  tNode *nd;

  bool debug;

  // Use thie to store probabilties for the resulting genotypes
  // implemented with double precision in angsd
  double geno_likes[10];
  
  // Probability matrix that a base is from a given geno-type
  float m_base_geno[4][10];

  // Contains q score corrections
  static const float qscore_corr[61];

  // model parameters
  static const float parlist[18];
  static const float pararray[2][2][4][4];

  void init_p_base();

 public : 

  // Use this to store the probabilities for the individual bases
  float base_prob[4];
 
  // Default constructor
  phys_genolike_calc();

  // Default destructor
  ~phys_genolike_calc();

  // 
  void update_tNode( tNode *curr_nd );

  void update_chunkyT( chunkyT *curr_chk );

  void update_pbase( int depth );

  void get_genolikes( int site, int sample, double *return_likes );

  // Helper functions to get results
  void get_base_prob_str( char *results_str );
  void get_genolikes_str( char *results_str );

  void set_debug( bool db );


};

#endif