//
//  IRWEN.cpp
//  pense
//
//  Created by David Kepplinger on 2017-05-30.
//  Copyright © 2017 David Kepplinger. All rights reserved.
//
#include "IRWEN.hpp"

#include <RcppArmadillo.h>
#include <Rmath.h>

#include "ElasticNet.hpp"
#include "olsreg.h"

static const int DEFAULT_OPT_MAXIT = 1000;
static const double DEFAULT_OPT_EPS = 1e-6;

static const double NUMERICAL_TOLERANCE = NUMERIC_EPS;

IRWEN::IRWEN(const Data& data, const double alpha, const double lambda, const Options& opts, const Options &enOpts) :
    data(data),
    maxIt(opts.get("maxit", DEFAULT_OPT_MAXIT)),
    eps(opts.get("eps", DEFAULT_OPT_EPS) * opts.get("eps", DEFAULT_OPT_EPS))
{
    Options overrideWarm;
    overrideWarm.set("warmStart", true);
    this->weights = new double[data.numObs()];
    this->en = getElasticNetImpl(enOpts, true);
    this->en->setOptions(overrideWarm);
    this->en->setAlphaLambda(alpha, lambda);
    this->en->setData(data);
}

IRWEN::~IRWEN()
{
    delete[] this->weights;
    delete this->en;
}

void IRWEN::compute(double *RESTRICT currentCoef, double *RESTRICT residuals)
{
    double tmp;
    double *RESTRICT oldCoef = new double[this->data.numVar()];
    double norm2Old;
    int j;

    this->iteration = 0;
    computeResiduals(this->data.getXtrConst(), this->data.getYConst(), this->data.numObs(),
                     this->data.numVar(), currentCoef, residuals);

    do {
        ++this->iteration;

        this->updateWeights(residuals);

        /*
         * Copy current coefficients to check for convergence later
         */
        memcpy(oldCoef, currentCoef, this->data.numVar() * sizeof(double));

        /*
         * Perform EN using current coefficients as warm start (only applicable for the coordinate
         * descent algorithm)
         */
        en->computeCoefsWeighted(currentCoef, residuals, this->weights);

        if (en->getStatus() > 0) {
            Rcpp::warning("Weighted elastic net had non-successful status:");
        }

        /*
         * Compute relative change
         */
        this->relChangeVal = 0;
        norm2Old = 0;
        for (j = 0; j < data.numVar(); ++j) {
            tmp = oldCoef[j] - currentCoef[j];
            this->relChangeVal += tmp * tmp;
            norm2Old += oldCoef[j] * oldCoef[j];
        }

        if (norm2Old < NUMERICAL_TOLERANCE) {
            if (this->relChangeVal < NUMERICAL_TOLERANCE) {
                /* We barely moved away from the zero-vector --> "converged" */
                this->relChangeVal = 0;
            } else {
                /* We moved away from the zero-vector --> continue */
                this->relChangeVal = 2 * this->eps;
            }
        } else {
            this->relChangeVal /= norm2Old;
        }

#ifdef DEBUG
        Rcpp::Rcout << "Rel. change: " << this->relChangeVal << std::endl;
#endif

    } while((this->iteration < this->maxIt) && (this->relChangeVal > this->eps));

    delete[] oldCoef;
}
