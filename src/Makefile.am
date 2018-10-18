## Source directory

bin_PROGRAMS = EvoFoldV2 grammarTrain dfgEval dfgTrain multinomial

LDADD = $(top_srcdir)/phy/libphy.la -lboost_program_options -llapack -lblas -lntl -lopt -lnewmat -lm -lgfortran -lpthread

EvoFoldV2_SOURCES    = EvoFold.cpp
grammarTrain_SOURCES = grammarTrain.cpp
dfgEval_SOURCES      = dfgEval.cpp
dfgTrain_SOURCES     = dfgTrain.cpp
multinomial_SOURCES  = multinomial.cpp

#  compiler options
AM_CPPFLAGS = -I$(top_srcdir)
