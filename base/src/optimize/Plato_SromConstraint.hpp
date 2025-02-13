/*
 * Plato_SromConstraint.hpp
 *
 *  Created on: Jan 31, 2018
 */

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
// *************************************************************************
//@HEADER
*/

#ifndef PLATO_SROMCONSTRAINT_HPP_
#define PLATO_SROMCONSTRAINT_HPP_

#include <memory>

#include "Plato_Vector.hpp"
#include "Plato_Criterion.hpp"
#include "Plato_MultiVector.hpp"
#include "Plato_LinearAlgebra.hpp"
#include "Plato_ReductionOperations.hpp"

namespace Plato
{

template<typename ScalarType, typename OrdinalType = size_t>
class SromConstraint : public Plato::Criterion<ScalarType, OrdinalType>
{
public:
    SromConstraint(const std::shared_ptr<Plato::ReductionOperations<ScalarType, OrdinalType>> & aReductionOperations) :
            mReductionOperations(aReductionOperations)
    {
    }
    virtual ~SromConstraint()
    {
    }

    void cacheData()
    {
        return;
    }
    ScalarType value(const Plato::MultiVector<ScalarType, OrdinalType> & aControl)
    {
        const OrdinalType tVectorIndex = aControl.getNumVectors() - static_cast<OrdinalType>(1);
        const Plato::Vector<ScalarType, OrdinalType> & tProbabilities = aControl[tVectorIndex];
        ScalarType tSum = mReductionOperations->sum(tProbabilities);
        ScalarType tOutput = tSum - static_cast<ScalarType>(1);
        return (tOutput);
    }
    void gradient(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                  Plato::MultiVector<ScalarType, OrdinalType> & aOutput)
    {
        const OrdinalType tNumDimensions = aControl.getNumVectors() - static_cast<OrdinalType>(1);
        for(OrdinalType tIndex = 0; tIndex < tNumDimensions; tIndex++)
        {
            Plato::Vector<ScalarType, OrdinalType> & tMySamplesGradient = aOutput[tIndex];
            tMySamplesGradient.fill(static_cast<ScalarType>(0));
        }
        Plato::Vector<ScalarType, OrdinalType> & tMyProbabilityGradient = aOutput[tNumDimensions];
        tMyProbabilityGradient.fill(static_cast<ScalarType>(1));
    }
    void hessian(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                 const Plato::MultiVector<ScalarType, OrdinalType> & aVector,
                 Plato::MultiVector<ScalarType, OrdinalType> & aOutput)
    {
        Plato::fill(static_cast<ScalarType>(0), aOutput);
    }

private:
    std::shared_ptr<Plato::ReductionOperations<ScalarType, OrdinalType>> mReductionOperations;

private:
    SromConstraint(const Plato::SromConstraint<ScalarType, OrdinalType> & aRhs);
    Plato::SromConstraint<ScalarType, OrdinalType> & operator=(const Plato::SromConstraint<ScalarType, OrdinalType> & aRhs);
};

} // namespace Plato

#endif /* BASE_SRC_OPTIMIZE_PLATO_SROMCONSTRAINT_HPP_ */
