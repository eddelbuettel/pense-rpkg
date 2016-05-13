//
//  ElasticNet.hpp
//  penseinit
//
//  Created by David Kepplinger on 2016-01-31.
//  Copyright © 2016 David Kepplinger. All rights reserved.
//

#ifndef ElasticNet_hpp
#define ElasticNet_hpp

#include "config.h"

#include <RcppArmadillo.h>

#include "Control.h"
#include "Data.hpp"


/**
 * Solve the EN problem
 *
 * (1 / 2N) * RSS + lambda * (((1 - alpha) / 2) * L2(beta)^2 + alpha * L1(beta))
 *
 */
class ElasticNet
{
public:
	ElasticNet(const double eps, const bool center) : center(center), eps(eps)
	{
	}

	virtual ~ElasticNet()
    {}

	/**
	 * Set the regularization based on one parameter for the L1 penalization
     * and one for the L2 penalization.
	 *
	 * NOTE: The values for lambda1 and lambda2 are INDEPENDENT
	 *		 of the number of observations!
	 *
	 */
	virtual void setLambdas(const double lambda1, const double lambda2) = 0;

	/**
	 * Set the regularization based on alpha and lambda
	 *
	 * NOTE: The values for lambda1 and lambda2 are INDEPENDENT
	 *		 of the number of observations!
	 *
	 */
	virtual void setAlphaLambda(const double alpha, const double lambda) = 0;

	void setThreshold(const double eps)
	{
		this->eps = eps;
	}

    /**
     * Solve the EN problem
     *
     * (1 / 2N) * RSS + lambda * (((1 - alpha) / 2) * L2(beta)^2 + alpha * L1(beta))
     *
     *
     * @param data Is assumed to have the leading column of 1's for the intercept
     * @param coefs Is assumed to be at least data.numVar() long! This can be taken as
     *          the starting point for the coordinate-descend algorithm. (see argument warm)
     * @param residuals A vector of residuals with as many observations as in data. The
     *          residuals will be recalculated for the given (warm) coefficients
     *
     * NOTE: The leading column of X is used as weight for the row in
     *		 in centering the data.
     *
     * @returns TRUE if the algorithm converged, FALSE otherwise
     */
    virtual bool computeCoefs(const Data& data, double *RESTRICT coefs,
							  double *RESTRICT residuals, const bool warm = FALSE) = 0;

protected:
    const bool center;
    double eps;
};




class ElasticNetGDESC : public ElasticNet
{
public:
	ElasticNetGDESC(const int maxIt, const double eps, const bool center);
	~ElasticNetGDESC();

	void setLambdas(const double lambda1, const double lambda2);

	void setAlphaLambda(const double alpha, const double lambda);

	/*
	 *
	 * @returns TRUE if the algorithm converged, FALSE otherwise
	 */
    bool computeCoefs(const Data& data, double *RESTRICT coefs,
							  double *RESTRICT residuals, const bool warm = FALSE);

private:
	const int maxIt;
	double alpha;
	double lambda;

	void resizeBuffer(const Data& data);

	double *RESTRICT Xtr;
	double *RESTRICT Xmeans;
	double *RESTRICT Xvars;
	int XtrSize;
	int XmeansSize;
};





class ElasticNetLARS : public ElasticNet {
public:
	enum UseGram {
		AUTO = 0,
		NO = 1,
		YES = 2
	};

    ElasticNetLARS(const double eps, const bool center, const UseGram useGram = AUTO);
    ~ElasticNetLARS();

	void setLambdas(const double lambda1, const double lambda2);

	void setAlphaLambda(const double alpha, const double lambda);

	void useGram(const UseGram useGram)
	{
		this->gramMode = useGram;
	}

    /**
     * @return Always returns TRUE as this algorithm is not iterative
     */
    bool computeCoefs(const Data& data, double *RESTRICT coefs, double *RESTRICT residuals,
                      const bool warm = FALSE);


private:
    /**
     * Automatically switch to non-Gram when more than the following
     * number of predictors are present.
     * 1400 ~ 15 MiByte of memory -- should be okay for most systems
     */
	static const int MAX_PREDICTORS_GRAM = 1400;

	double lambda1;
	double sqrtLambda2;
	UseGram gramMode;


	arma::mat XtrAug;
	arma::vec yAug;
	arma::uword augNobs;

	/**
	 * Certain matrices/vectors which are useful to store
	 * between calls if the size of the data doesn't change too often.
	 */
	arma::mat gramMat;
	arma::vec corY;
	arma::vec meanX;

	void augmentData(const Data& data);

};




/**
 * Convenience functions to choose one of the above Elastic Net implementation.
 */
ElasticNet* getElasticNetImpl(const ENAlgorithm enAlgorithm, const double eps, const bool center,
							  const int maxIt);
ElasticNet* getElasticNetImpl(const Control& ctrl);


#endif /* ElasticNet_hpp */
