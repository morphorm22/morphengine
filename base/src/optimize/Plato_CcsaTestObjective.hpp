/*
//@HEADER
// *************************************************************************
//   Plato Engine v.1.0: Copyright 2018, National Technology & Engineering
//                    Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Sandia Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact the Plato team (plato3D-help@sandia.gov)
//
// *************************************************************************
//@HEADER
*/

/*
 * Plato_CcsaTestObjective.hpp
 *
 *  Created on: Nov 4, 2017
 */

#pragma once

#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>

#include "Plato_Vector.hpp"
#include "Plato_Criterion.hpp"
#include "Plato_MultiVector.hpp"
#include "Plato_LinearAlgebra.hpp"
#include "Plato_StandardVectorReductionOperations.hpp"

namespace Plato
{

/******************************************************************************//**
 * @brief Criterion used to test Method of Moving Asymptotes (MMA) optimizer. The
 * criterion /f$ \f(x) /f$ is defined as /f$ \alpha\sum_{i=1}^{N} x_i /f$, where
 * /f$ \alpha /f$ is a constant set to 0.0624 and N is the number of optimization
 * variables.
**********************************************************************************/
template<typename ScalarType, typename OrdinalType = size_t>
class CcsaTestObjective : public Plato::Criterion<ScalarType, OrdinalType>
{
public:
    /******************************************************************************//**
     * @brief Constructor
    **********************************************************************************/
    CcsaTestObjective() :
            mWeightConstant(0.0624),
            mReduction(std::make_shared<Plato::StandardVectorReductionOperations<ScalarType,OrdinalType>>())
    {
    }

    /******************************************************************************//**
     * @brief Destructor
    **********************************************************************************/
    virtual ~CcsaTestObjective()
    {
    }

    /******************************************************************************//**
     * @brief Set weight constant
     * @param [in] aInput weight constant
    **********************************************************************************/
    void setWeightConstant(const ScalarType& aInput)
    {
        mWeightConstant = aInput;
    }

    /******************************************************************************//**
     * @brief Cache criterion specific data.
    **********************************************************************************/
    void cacheData()
    {
        return;
    }

    /******************************************************************************//**
     * @brief Evaluate objective function
     * @param [in] aControl optimization variables
     * @return objective function value
    **********************************************************************************/
    ScalarType value(const Plato::MultiVector<ScalarType, OrdinalType> & aControl)
    {
        assert(aControl.getNumVectors() > static_cast<OrdinalType>(0));

        OrdinalType tNumVectors = aControl.getNumVectors();
        std::vector<ScalarType> tStorage(tNumVectors);
        for(OrdinalType tVectorIndex = 0; tVectorIndex < tNumVectors; tVectorIndex++)
        {
            const Plato::Vector<ScalarType, OrdinalType> & tMyControl = aControl[tVectorIndex];
            tStorage[tVectorIndex] = mReduction->sum(tMyControl);
        }
        const ScalarType tInitialValue = 0;
        ScalarType tSum = std::accumulate(tStorage.begin(), tStorage.end(), tInitialValue);
        ScalarType tOutput = mWeightConstant * tSum;

        return (tOutput);
    }

    /******************************************************************************//**
     * @brief Compute objective function gradient
     * @param [in] aControl optimization variables
     * @param [in/out] aOutput objective function gradient
    **********************************************************************************/
    void gradient(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                  Plato::MultiVector<ScalarType, OrdinalType> & aOutput)
    {
        Plato::fill(mWeightConstant, aOutput);
    }

    /******************************************************************************//**
     * @brief Apply descent direction to Hessian
     * @param [in] aControl optimization variables
     * @param [in] aVector descent direction
     * @param [in/out] aOutput application of descent direction to Hessian
    **********************************************************************************/
    void hessian(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                 const Plato::MultiVector<ScalarType, OrdinalType> & aVector,
                 Plato::MultiVector<ScalarType, OrdinalType> & aOutput)
    {
        const ScalarType tScalarValue = 0;
        const OrdinalType tVectorIndex = 0;
        aOutput[tVectorIndex].fill(tScalarValue);
    }

private:
    ScalarType mWeightConstant; /*!< scalar constant used in objective function */
    std::shared_ptr<Plato::ReductionOperations<ScalarType, OrdinalType>> mReduction; /*!< interface to reduction operations */

private:
    CcsaTestObjective(const Plato::CcsaTestObjective<ScalarType, OrdinalType> & aRhs);
    Plato::CcsaTestObjective<ScalarType, OrdinalType> & operator=(const Plato::CcsaTestObjective<ScalarType, OrdinalType> & aRhs);
};
// class CcsaTestObjective

}
// namespace Plato
