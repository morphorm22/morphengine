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
 * Plato_ParticleSwarmStageMngBCPSO.hpp
 *
 *  Created on: Jan 23, 2019
 */

#pragma once

#include "Plato_DataFactory.hpp"
#include "Plato_GradFreeCriterion.hpp"
#include "Plato_ParticleSwarmDataMng.hpp"
#include "Plato_ParticleSwarmStageMng.hpp"

namespace Plato
{

/******************************************************************************//**
 * @brief Bound Constrained Particle Swarm Optimization (BCPSO) algorithm stage
 *        managers. The BCPSO stage manages calls to criteria (e.g. objective functions).
**********************************************************************************/
template<typename ScalarType, typename OrdinalType = size_t>
class ParticleSwarmStageMngBCPSO : public Plato::ParticleSwarmStageMng<ScalarType, OrdinalType>
{
public:
    /******************************************************************************//**
     * @brief Constructor
     * @param [in] aFactory data factory
     * @param [in] aObjective gradient free objective function interface
    **********************************************************************************/
    explicit ParticleSwarmStageMngBCPSO(const std::shared_ptr<Plato::DataFactory<ScalarType, OrdinalType>> & aFactory,
                                         const std::shared_ptr<Plato::GradFreeCriterion<ScalarType, OrdinalType>> & aObjective) :
            mCurrentObjFuncValues(aFactory->objectiveFunctionVals().create()),
            mObjective(aObjective)
    {
    }

    /******************************************************************************//**
     * @brief Destructor
    **********************************************************************************/
    virtual ~ParticleSwarmStageMngBCPSO()
    {
    }

    /******************************************************************************//**
     * @brief Evaluate objective function
     * @param [in,out] aDataMng PSO data manager
    **********************************************************************************/
    void evaluateObjective(Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & aDataMng)
    {
        mCurrentObjFuncValues->fill(static_cast<ScalarType>(0));
        const Plato::MultiVector<ScalarType, OrdinalType> & tParticles = aDataMng.getCurrentParticles();
        mObjective->value(tParticles, *mCurrentObjFuncValues);
        aDataMng.setCurrentObjFuncValues(*mCurrentObjFuncValues);
    }

    /******************************************************************************//**
     * @brief Find current best particle positions
     * @param [in,out] aDataMng PSO data manager
    **********************************************************************************/
    void findBestParticlePositions(Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & aDataMng)
    {
        aDataMng.cacheGlobalBestObjFunValue();
        aDataMng.updateBestParticlesData();
        aDataMng.findGlobalBestParticle();
    }

private:
    std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> mCurrentObjFuncValues; /*!< current objective function values */
    std::shared_ptr<Plato::GradFreeCriterion<ScalarType, OrdinalType>> mObjective; /*!< interface for grad-free objective function */

private:
    ParticleSwarmStageMngBCPSO(const Plato::ParticleSwarmStageMngBCPSO<ScalarType, OrdinalType>&);
    Plato::ParticleSwarmStageMngBCPSO<ScalarType, OrdinalType> & operator=(const Plato::ParticleSwarmStageMngBCPSO<ScalarType, OrdinalType>&);
};
// class ParticleSwarmStageMngBCPSO

} // namespace Plato
