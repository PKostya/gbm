//  GBM by Greg Ridgeway  Copyright (C) 2003

#include "poisson.h"

CPoisson::CPoisson()
{
}

CPoisson::~CPoisson()
{
}


void CPoisson::ComputeWorkingResponse
(
    double *adY,
    double *adMisc,
    double *adOffset,
    double *adF,
    double *adZ,
    double *adWeight,
    int *afInBag,
    unsigned long nTrain,
    int cIdxOff
)
{
    unsigned long i = 0;
    double dF = 0.0;

    // compute working response
    for(i=0; i < nTrain; i++)
    {
        dF = adF[i] + ((adOffset==NULL) ? 0.0 : adOffset[i]);
        adZ[i] = adY[i] - std::exp(dF);
    }
}



void CPoisson::InitF
(
    double *adY,
    double *adMisc,
    double *adOffset,
    double *adWeight,
    double &dInitF,
    unsigned long cLength
)
{
    double dSum = 0.0;
    double dDenom = 0.0;
    unsigned long i = 0;

    if(adOffset == NULL)
    {
        for(i=0; i<cLength; i++)
        {
            dSum += adWeight[i]*adY[i];
            dDenom += adWeight[i];
        }
    }
    else
    {
        for(i=0; i<cLength; i++)
        {
            dSum += adWeight[i]*adY[i];
            dDenom += adWeight[i]*std::exp(adOffset[i]);
        }
    }

    dInitF = std::log(dSum/dDenom);
}


double CPoisson::Deviance
(
    double *adY,
    double *adMisc,
    double *adOffset,
    double *adWeight,
    double *adF,
    unsigned long cLength,
	int cIdxOff
)
{
    unsigned long i=0;
    double dL = 0.0;
    double dW = 0.0;

    if(adOffset == NULL)
    {
        for(i=cIdxOff; i<cLength+cIdxOff; i++)
        {
            dL += adWeight[i]*(adY[i]*adF[i] - std::exp(adF[i]));
            dW += adWeight[i];
        }
    }
    else
    {
        for(i=cIdxOff; i<cLength+cIdxOff; i++)
        {
            dL += adWeight[i]*(adY[i]*(adOffset[i]+adF[i]) -
                               std::exp(adOffset[i]+adF[i]));
            dW += adWeight[i];
       }
    }

    return -2*dL/dW;
}


void CPoisson::FitBestConstant
(
    double *adY,
    double *adMisc,
    double *adOffset,
    double *adW,
    double *adF,
    double *adZ,
    const std::vector<unsigned long>& aiNodeAssign,
    unsigned long nTrain,
    VEC_P_NODETERMINAL vecpTermNodes,
    unsigned long cTermNodes,
    unsigned long cMinObsInNode,
    int *afInBag,
    double *adFadj,
    int cIdxOff
)
{
    unsigned long iObs = 0;
    unsigned long iNode = 0;
    vecdNum.resize(cTermNodes);
    vecdNum.assign(vecdNum.size(),0.0);
    vecdDen.resize(cTermNodes);
    vecdDen.assign(vecdDen.size(),0.0);

    vecdMax.resize(cTermNodes);
    vecdMax.assign(vecdMax.size(),-HUGE_VAL);
    vecdMin.resize(cTermNodes);
    vecdMin.assign(vecdMin.size(),HUGE_VAL);

    if(adOffset == NULL)
    {
        for(iObs=0; iObs<nTrain; iObs++)
        {
            if(afInBag[iObs])
            {
                vecdNum[aiNodeAssign[iObs]] += adW[iObs]*adY[iObs];
                vecdDen[aiNodeAssign[iObs]] += adW[iObs]*std::exp(adF[iObs]);
            }
            vecdMax[aiNodeAssign[iObs]] =
               R::fmax2(adF[iObs],vecdMax[aiNodeAssign[iObs]]);
            vecdMin[aiNodeAssign[iObs]] =
               R::fmin2(adF[iObs],vecdMin[aiNodeAssign[iObs]]);
        }
    }
    else
    {
        for(iObs=0; iObs<nTrain; iObs++)
        {
            if(afInBag[iObs])
            {
                vecdNum[aiNodeAssign[iObs]] += adW[iObs]*adY[iObs];
                vecdDen[aiNodeAssign[iObs]] +=
                    adW[iObs]*std::exp(adOffset[iObs]+adF[iObs]);
            }
        }
    }
    for(iNode=0; iNode<cTermNodes; iNode++)
    {
        if(vecpTermNodes[iNode]!=NULL)
        {
            if(vecdNum[iNode] == 0.0)
            {
                // DEBUG: if vecdNum==0 then prediction = -Inf
                // Not sure what else to do except plug in an arbitrary
                //   negative number, -1? -10? Let's use -1, then make
                //   sure |adF| < 19 always.
                vecpTermNodes[iNode]->dPrediction = -19.0;
            }
            else if(vecdDen[iNode] == 0.0)
            {
                vecpTermNodes[iNode]->dPrediction = 0.0;
            }
            else
            {
                vecpTermNodes[iNode]->dPrediction =
                    std::log(vecdNum[iNode]/vecdDen[iNode]);
            }
            vecpTermNodes[iNode]->dPrediction =
               R::fmin2(vecpTermNodes[iNode]->dPrediction,
                     19-vecdMax[iNode]);
            vecpTermNodes[iNode]->dPrediction =
               R::fmax2(vecpTermNodes[iNode]->dPrediction,
                     -19-vecdMin[iNode]);
        }
    }
}


double CPoisson::BagImprovement
(
    double *adY,
    double *adMisc,
    double *adOffset,
    double *adWeight,
    double *adF,
    double *adFadj,
    int *afInBag,
    double dStepSize,
    unsigned long nTrain
)
{
    double dReturnValue = 0.0;
    double dF = 0.0;
    double dW = 0.0;
    unsigned long i = 0;

    for(i=0; i<nTrain; i++)
    {
        if(!afInBag[i])
        {
            dF = adF[i] + ((adOffset==NULL) ? 0.0 : adOffset[i]);

            dReturnValue += adWeight[i]*
                            (adY[i]*dStepSize*adFadj[i] -
                             std::exp(dF+dStepSize*adFadj[i]) +
                             std::exp(dF));
            dW += adWeight[i];
        }
    }

    return dReturnValue/dW;
}


