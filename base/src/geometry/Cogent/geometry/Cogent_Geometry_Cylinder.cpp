/*
 * Cogent_Geometry_Cylinder.cpp
 *
 *  Created on: Nov 23, 2016
 *      Author: jrobbin
 */

#include "Cogent_Geometry_Cylinder.hpp"
#include "Cogent_Geometry_CylinderSurface.hpp"
#include "Cogent_Geometry_Plane.hpp"

namespace Cogent {
namespace Geometry {

/******************************************************************************/
Cylinder::
Cylinder(const Teuchos::ParameterList& params,
         const std::vector<std::string>& shapeParameterNames,
         Teuchos::RCP<Cogent::CoordinateSystem> coordinateSystem) :
  Primitive(params, shapeParameterNames, coordinateSystem,
  /*numDim*/3, /*numLevelsets*/3, /*numLocalParams*/2)
/******************************************************************************/
{
  m_boundaryNames.resize(c_numLevelsets);

  std::string strRadius = params.get<std::string>("Radius");
  m_localParameters.push_back(Teuchos::rcp(new Cogent::ParameterFunction(shapeParameterNames, strRadius)));

  std::string strHeight = params.get<std::string>("Height");
  m_localParameters.push_back(Teuchos::rcp(new Cogent::ParameterFunction(shapeParameterNames, strHeight)));

  //                        0       1   
  // localParameters are {radius, height}

  std::string axisString = params.get<std::string>("Axis");
  if(axisString == "X" || axisString == "x"){ m_axis = Axis::X;}
   else
  if(axisString == "Y" || axisString == "y"){ m_axis = Axis::Y;}
  else
  if(axisString == "Z" || axisString == "z"){ m_axis = Axis::Z;}
  else
    TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error, std::endl <<
      "Cogent::Geometry::Cylinder: specify X, Y, or Z" << std::endl);

  // cylinder wall
  m_boundaryNames[0] = "Cylinder Wall";
  m_boundaries.push_back(Teuchos::rcp(new CylinderSurface(m_localParameters, Parameter::Radius, m_axis)));
  // negative plane end
  m_boundaryNames[1] = "Negative Cap";
  m_boundaries.push_back(Teuchos::rcp(new Plane(m_localParameters, Parameter::Height, 
                                                -1.0/2.0, m_axis, Sense::Positive)));
  // positive plane end
  m_boundaryNames[2] = "Positive Cap";
  m_boundaries.push_back(Teuchos::rcp(new Plane(m_localParameters, Parameter::Height,
                                                 1.0/2.0, m_axis, Sense::Negative)));
  
  // fill aliases
  for( auto& alias : m_boundaryAliases ){
    bool foundBoundary = false;
    for(int iBoundary=0; iBoundary<c_numLevelsets; iBoundary++){
      if( alias.boundaryName == m_boundaryNames[iBoundary]){
        alias.localIndex = iBoundary;
        foundBoundary = true;
        break;
      }
    }
    TEUCHOS_TEST_FOR_EXCEPTION(!foundBoundary, std::runtime_error,
      std::endl << "Boundary with name '" << alias.boundaryName << "' not found" << std::endl);
  }
}

Cylinder::~Cylinder() { }

} /* namespace Geometry */
} /* namespace Cogent */
