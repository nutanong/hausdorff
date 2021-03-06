// Spatial Index Library
//
// Copyright (C) 2004  Navel Ltd.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Email:
//    mhadji@gmail.com

#include "../../include/SpatialIndex.h"

#include <cstring>
#include <cmath>
#include <limits>
#include <vector>

using namespace SpatialIndex;

Region::Region()
	: m_dimension(0), m_pLow(0), m_pHigh(0)
{
}

Region::Region(uint32_t d)
{
	initialize(d);
}

Region::Region(const double* pLow, const double* pHigh, uint32_t dimension)
{
	initialize(pLow, pHigh, dimension);
}

Region::Region(const Point& low, const Point& high)
{
	if (low.m_dimension != high.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::Region: arguments have different number of dimensions."
		);

	initialize(low.m_pCoords, high.m_pCoords, low.m_dimension);
}

Region::Region(const Region& r)
{
	initialize(r.m_pLow, r.m_pHigh, r.m_dimension);
	if (this->m_vec_pEdge.empty()) {
		for (int i=0; i < 4; i++) {
			Region *pR = new Region(2);
			this->getEdge(i,*pR);
			this->m_vec_pEdge.push_back(pR);
		}
	}
}

void Region::initialize(const double* pLow, const double* pHigh, uint32_t dimension)
{
	m_pLow = 0;
	m_dimension = dimension;

#ifndef NDEBUG
    for (uint32_t cDim = 0; cDim < m_dimension; ++cDim)
    {
     if ((pLow[cDim] > pHigh[cDim]))
     {
         // check for infinitive region
         if (!(pLow[cDim] == std::numeric_limits<double>::max() ||
             pHigh[cDim] == -std::numeric_limits<double>::max() ))
             throw Tools::IllegalArgumentException(
                 "Region::initialize: Low point has larger coordinates than High point."
                 " Neither point is infinity."
             );
     }
    }
#endif

	try
	{
		m_pLow = new double[m_dimension];
		m_pHigh = new double[m_dimension];
	}
	catch (...)
	{
		delete[] m_pLow;
		throw;
	}

	memcpy(m_pLow, pLow, m_dimension * sizeof(double));
	memcpy(m_pHigh, pHigh, m_dimension * sizeof(double));



}

void Region::initialize(uint32_t dimension)
{
	m_dimension = dimension;

	try
	{
		m_pLow = new double[m_dimension];
		m_pHigh = new double[m_dimension];
	}
	catch (...)
	{
		delete[] m_pLow;
		throw;
	}
}


Region::~Region()
{
	delete[] m_pLow;
	delete[] m_pHigh;

	for (int i=0; i < this->m_vec_pEdge.size(); i++) {
		delete this->m_vec_pEdge.at(i);
	}
}

Region& Region::operator=(const Region& r)
{
	if(this != &r)
	{
		makeDimension(r.m_dimension);
		memcpy(m_pLow, r.m_pLow, m_dimension * sizeof(double));
		memcpy(m_pHigh, r.m_pHigh, m_dimension * sizeof(double));
	}

	return *this;
}

bool Region::operator==(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::operator==: Regions have different number of dimensions."
		);

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (
			m_pLow[i] < r.m_pLow[i] - std::numeric_limits<double>::epsilon() ||
			m_pLow[i] > r.m_pLow[i] + std::numeric_limits<double>::epsilon() ||
			m_pHigh[i] < r.m_pHigh[i] - std::numeric_limits<double>::epsilon() ||
			m_pHigh[i] > r.m_pHigh[i] + std::numeric_limits<double>::epsilon())
			return false;
	}
	return true;
}

//
// IObject interface
//
Region* Region::clone()
{
	return new Region(*this);
}

//
// ISerializable interface
//
uint32_t Region::getByteArraySize()
{
	return (sizeof(uint32_t) + 2 * m_dimension * sizeof(double));
}

void Region::loadFromByteArray(const byte* ptr)
{
	uint32_t dimension;
	memcpy(&dimension, ptr, sizeof(uint32_t));
	ptr += sizeof(uint32_t);

	makeDimension(dimension);
	memcpy(m_pLow, ptr, m_dimension * sizeof(double));
	ptr += m_dimension * sizeof(double);
	memcpy(m_pHigh, ptr, m_dimension * sizeof(double));
	//ptr += m_dimension * sizeof(double);
}

void Region::storeToByteArray(byte** data, uint32_t& len)
{
	len = getByteArraySize();
	*data = new byte[len];
	byte* ptr = *data;

	memcpy(ptr, &m_dimension, sizeof(uint32_t));
	ptr += sizeof(uint32_t);
	memcpy(ptr, m_pLow, m_dimension * sizeof(double));
	ptr += m_dimension * sizeof(double);
	memcpy(ptr, m_pHigh, m_dimension * sizeof(double));
	//ptr += m_dimension * sizeof(double);
}

//
// IShape interface
//
bool Region::intersectsShape(const IShape& s) const
{
	const Region* pr = dynamic_cast<const Region*>(&s);
	if (pr != 0) return intersectsRegion(*pr);

	const Point* ppt = dynamic_cast<const Point*>(&s);
	if (ppt != 0) return containsPoint(*ppt);

	throw Tools::IllegalStateException(
		"Region::intersectsShape: Not implemented yet!"
	);
}

bool Region::containsShape(const IShape& s) const
{
	const Region* pr = dynamic_cast<const Region*>(&s);
	if (pr != 0) return containsRegion(*pr);

	const Point* ppt = dynamic_cast<const Point*>(&s);
	if (ppt != 0) return containsPoint(*ppt);

	throw Tools::IllegalStateException(
		"Region::containsShape: Not implemented yet!"
	);
}

bool Region::touchesShape(const IShape& s) const
{
	const Region* pr = dynamic_cast<const Region*>(&s);
	if (pr != 0) return touchesRegion(*pr);

	const Point* ppt = dynamic_cast<const Point*>(&s);
	if (ppt != 0) return touchesPoint(*ppt);

	throw Tools::IllegalStateException(
		"Region::touchesShape: Not implemented yet!"
	);
}

void Region::getCenter(Point& out) const
{
	out.makeDimension(m_dimension);
	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		out.m_pCoords[i] = (m_pLow[i] + m_pHigh[i]) / 2.0;
	}
}

uint32_t Region::getDimension() const
{
	return m_dimension;
}

void Region::getMBR(Region& out) const
{
	out = *this;


	out.m_vec_pEdge.clear();
	for (int i=0; i < 4; i++) {
		Region *pR = new Region(2);
		out.getEdge(i,*pR);
		out.m_vec_pEdge.push_back(pR);
	}
}

double Region::getArea() const
{
	double area = 1.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		area *= m_pHigh[i] - m_pLow[i];
	}

	return area;
}

double Region::getMinimumDistance(const IShape& s) const
{
	const Region* pr = dynamic_cast<const Region*>(&s);
	if (pr != 0) return getMinimumDistance(*pr);

	const Point* ppt = dynamic_cast<const Point*>(&s);
	if (ppt != 0) return getMinimumDistance(*ppt);

	throw Tools::IllegalStateException(
		"Region::getMinimumDistance: Not implemented yet!"
	);
}



bool Region::intersectsRegion(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::intersectsRegion: Regions have different number of dimensions."
		);

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (m_pLow[i] > r.m_pHigh[i] || m_pHigh[i] < r.m_pLow[i]) return false;
	}
	return true;
}

bool Region::containsRegion(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::containsRegion: Regions have different number of dimensions."
		);

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (m_pLow[i] > r.m_pLow[i] || m_pHigh[i] < r.m_pHigh[i]) return false;
	}
	return true;
}

bool Region::touchesRegion(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::touchesRegion: Regions have different number of dimensions."
		);
	
	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (
			(m_pLow[i] >= r.m_pLow[i] + std::numeric_limits<double>::epsilon() &&
			m_pLow[i] <= r.m_pLow[i] - std::numeric_limits<double>::epsilon()) ||
			(m_pHigh[i] >= r.m_pHigh[i] + std::numeric_limits<double>::epsilon() &&
			m_pHigh[i] <= r.m_pHigh[i] - std::numeric_limits<double>::epsilon()))
			return false;
	}
	return true;
}

double Region::getMinimumDistance(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getMinimumDistance: Regions have different number of dimensions."
		);

	double ret = 0.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		double x = 0.0;

		if (r.m_pHigh[i] < m_pLow[i])
		{
			x = std::abs(r.m_pHigh[i] - m_pLow[i]);
		}
		else if (m_pHigh[i] < r.m_pLow[i])
		{
			x = std::abs(r.m_pLow[i] - m_pHigh[i]);
		}

		ret += x * x;
	}

	return std::sqrt(ret);
}

bool Region::containsPoint(const Point& p) const
{
	if (m_dimension != p.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::containsPoint: Point has different number of dimensions."
		);

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (m_pLow[i] > p.getCoordinate(i) || m_pHigh[i] < p.getCoordinate(i)) return false;
	}
	return true;
}

bool Region::touchesPoint(const Point& p) const
{
	if (m_dimension != p.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::touchesPoint: Point has different number of dimensions."
		);

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (
			(m_pLow[i] >= p.getCoordinate(i) - std::numeric_limits<double>::epsilon() &&
			 m_pLow[i] <= p.getCoordinate(i) + std::numeric_limits<double>::epsilon()) ||
			(m_pHigh[i] >= p.getCoordinate(i) - std::numeric_limits<double>::epsilon() &&
			 m_pHigh[i] <= p.getCoordinate(i) + std::numeric_limits<double>::epsilon()))
			return true;
	}
	return false;
}

double Region::getMinimumDistance(const Point& p) const
{
	if (m_dimension != p.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getMinimumDistance: Point has different number of dimensions."
		);

	double ret = 0.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (p.getCoordinate(i) < m_pLow[i])
		{
			ret += std::pow(m_pLow[i] - p.getCoordinate(i), 2.0);
		}
		else if (p.getCoordinate(i) > m_pHigh[i])
		{
			ret += std::pow(p.getCoordinate(i) - m_pHigh[i], 2.0);
		}
	}

	return std::sqrt(ret);
}

Region Region::getIntersectingRegion(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getIntersectingRegion: Regions have different number of dimensions."
		);

	Region ret;
	ret.makeInfinite(m_dimension);

	// check for intersection.
	// marioh: avoid function call since this is called billions of times.
	for (uint32_t cDim = 0; cDim < m_dimension; ++cDim)
	{
		if (m_pLow[cDim] > r.m_pHigh[cDim] || m_pHigh[cDim] < r.m_pLow[cDim]) return ret;
	}

	for (uint32_t cDim = 0; cDim < m_dimension; ++cDim)
	{
		ret.m_pLow[cDim] = std::max(m_pLow[cDim], r.m_pLow[cDim]);
		ret.m_pHigh[cDim] = std::min(m_pHigh[cDim], r.m_pHigh[cDim]);
	}

	return ret;
}

double Region::getIntersectingArea(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getIntersectingArea: Regions have different number of dimensions."
		);

	double ret = 1.0;
	double f1, f2;

	for (uint32_t cDim = 0; cDim < m_dimension; ++cDim)
	{
		if (m_pLow[cDim] > r.m_pHigh[cDim] || m_pHigh[cDim] < r.m_pLow[cDim]) return 0.0;

		f1 = std::max(m_pLow[cDim], r.m_pLow[cDim]);
		f2 = std::min(m_pHigh[cDim], r.m_pHigh[cDim]);
		ret *= f2 - f1;
	}

	return ret;
}

/*
 * Returns the margin of a region. It is calcuated as the sum of  2^(d-1) * width, in each dimension.
 * It is actually the sum of all edges, no matter what the dimensionality is.
*/
double Region::getMargin() const
{
	double mul = std::pow(2.0, static_cast<double>(m_dimension) - 1.0);
	double margin = 0.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		margin += (m_pHigh[i] - m_pLow[i]) * mul;
	}

	return margin;
}

void Region::combineRegion(const Region& r)
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::combineRegion: Region has different number of dimensions."
		);

	for (uint32_t cDim = 0; cDim < m_dimension; ++cDim)
	{
		m_pLow[cDim] = std::min(m_pLow[cDim], r.m_pLow[cDim]);
		m_pHigh[cDim] = std::max(m_pHigh[cDim], r.m_pHigh[cDim]);
	}
}

void Region::combinePoint(const Point& p)
{
	if (m_dimension != p.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::combinePoint: Point has different number of dimensions."
		);

	for (uint32_t cDim = 0; cDim < m_dimension; ++cDim)
	{
		m_pLow[cDim] = std::min(m_pLow[cDim], p.m_pCoords[cDim]);
		m_pHigh[cDim] = std::max(m_pHigh[cDim], p.m_pCoords[cDim]);
	}
}

void Region::getCombinedRegion(Region& out, const Region& in) const
{
	if (m_dimension != in.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getCombinedRegion: Regions have different number of dimensions."
		);

	out = *this;
	out.combineRegion(in);
}

double Region::getLow(uint32_t index) const
{
	if (index < 0 || index >= m_dimension)
		throw Tools::IndexOutOfBoundsException(index);

	return m_pLow[index];
}

double Region::getHigh(uint32_t index) const
{
	if (index < 0 || index >= m_dimension)
		throw Tools::IndexOutOfBoundsException(index);

	return m_pHigh[index];
}

void Region::makeInfinite(uint32_t dimension)
{
	makeDimension(dimension);
	for (uint32_t cIndex = 0; cIndex < m_dimension; ++cIndex)
	{
		m_pLow[cIndex] = std::numeric_limits<double>::max();
		m_pHigh[cIndex] = -std::numeric_limits<double>::max();
	}
}

void Region::makeDimension(uint32_t dimension)
{
	if (m_dimension != dimension)
	{
		delete[] m_pLow;
		delete[] m_pHigh;

		// remember that this is not a constructor. The object will be destructed normally if
		// something goes wrong (bad_alloc), so we must take care not to leave the object at an intermediate state.
		m_pLow = 0; m_pHigh = 0;

		m_dimension = dimension;
		m_pLow = new double[m_dimension];
		m_pHigh = new double[m_dimension];
	}
}

std::ostream& SpatialIndex::operator<<(std::ostream& os, const Region& r)
{
	uint32_t i;

	os << "Low: ";
	for (i = 0; i < r.m_dimension; ++i)
	{
		os << r.m_pLow[i] << " ";
	}

	os << ", High: ";

	for (i = 0; i < r.m_dimension; ++i)
	{
		os << r.m_pHigh[i] << " ";
	}

	return os;
}


/*
 *  Added by Yi 5/19/2011
 * 	Computing HausDistLB from an MBR to another MBR or a point.
 */

double Region::getHausDistLB(Region r) const
{
	if (this->m_dimension != 2) {
		throw Tools::NotSupportedException(
			"Region::getHausDistUB: #dimensions not supported"
		);
	}

	//Region edge1 = Region(2);
	//Region r = Region(2);
	//s.getMBR(r);
	//std::cout << "s: " << r.m_pLow[0] << " " << r.m_pLow[1] << " " << r.m_pHigh[0] << " " << r.m_pHigh[1] << std::endl;
	double max = std::numeric_limits<double>::min();
	for (int i=0; i<4; i++) {
		//this->getEdge(i,edge1);
		max = std::max(max, this->m_vec_pEdge[i]->getMinimumDistanceSq(r));
		//std::cout << "e: " << edge1.m_pLow[0] << " " << edge1.m_pLow[1] << " " << edge1.m_pHigh[0] << " " << edge1.m_pHigh[1] << std::endl;
		//std::cout << "Distance: " << std::sqrt(edge1.getMinimumDistanceSq(s)) << std::endl;
	}

	return std::sqrt(max);
}

double Region::getHausDistLB(const std::vector<const Region*> vec_pMBR, double max, int& counter) const
{

	//if (this->m_dimension != 2) {
		//throw Tools::NotSupportedException(
			//"Region::getHausDistUB: #dimensions not supported"
		//);
	//}

	max = max*max;
	//Region edge1 = Region(2);


	for (int i=0; i<4; i++) {
		//this->getEdge(i,edge1);
		double min = std::numeric_limits<double>::max();

		for (int j=0; j<  vec_pMBR.size(); j++) {
			min = std::min(min,this->m_vec_pEdge.at(i)->getMinimumDistanceSq(*(vec_pMBR[j])));
			//min = std::min(min,edge1.getMinimumDistanceSq(*(vec_pMBR[j])));
			counter++;
			if (min < max) break;
		}

		max = std::max(max,min);
	}

	return std::sqrt(max);
}


/*
 *  Added by Marco 6/1/11
 * 	Computing MHausDistLB from an MBR to another MBR or a point.
 */

double Region::getMHausDistLB(const IShape& s) const
{
	if (this->m_dimension != 2) {
		throw Tools::NotSupportedException(
			"Region::getHausDistUB: #dimensions not supported"
		);
	}

  return std::sqrt(this->getMinimumDistanceSq(s));
}

double Region::getMHausDistLB(const std::vector<const Region*> vec_pMBR, double max) const
{

	if (this->m_dimension != 2) {
		throw Tools::NotSupportedException(
			"Region::getHausDistUB: #dimensions not supported"
		);
	}

  double min = std::numeric_limits<double>::max();

  for (int j=0; j<  vec_pMBR.size(); j++) {
    min = std::min(min,this->getMinimumDistanceSq(*(vec_pMBR[j])));
    //if (min==0) break;
  }

	return std::sqrt(min);
}


/*
 * Computing HausDistUB from an MBR to another MBR or a point.
 *
 */

double Region::getHausDistUB(const IShape& s) const
{
	const Region* pr = dynamic_cast<const Region*>(&s);
	if (pr != 0) return getHausDistUB(*pr);

	const Point* ppt = dynamic_cast<const Point*>(&s);
	if (ppt != 0) return getHausDistUB(*ppt);

	throw Tools::IllegalStateException(
		"Region::getHausDistUB: Not implemented yet!"
	);
}

double Region::getHausDistUB(const Region& s) const
{
	Region edge1 = Region(2);
	Region edge2 = Region(2);

	double max = std::numeric_limits<double>::min();
	for (int i=0; i<4; i++) {
		this->getEdge(i,edge1);
		double min = std::numeric_limits<double>::max();
		for (int j=0; j<4; j++) {
			this->getEdge(j,edge2);
			min = std::min(min,edge1.getMaximumDistanceSq(edge2));
		}
		max = std::max(max,min);
	}

	return std::sqrt(max);
}

double Region::getHausDistUB(const Point& s) const
{
	Point sw = Point(this->m_pLow, 2);
	Point ne = Point(this->m_pHigh, 2);

	double coor[2];
	coor[0] = this->m_pHigh[0];
	coor[1] = this->m_pLow[1];
	Point se = Point(coor, 2);

	coor[0] = this->m_pLow[0];
	coor[1] = this->m_pHigh[1];
	Point nw = Point(coor, 2);

	double dSq = s.getDistanceSq(sw);
	dSq = std::max(dSq,s.getDistanceSq(ne));
	dSq = std::max(dSq,s.getDistanceSq(se));
	dSq = std::max(dSq,s.getDistanceSq(nw));

	return std::sqrt(dSq);
}

double Region::getHausDistUB(const std::vector<const IShape*> vec_pShape) const
{

	if (this->m_dimension != 2) {
		throw Tools::NotSupportedException(
			"Region::getHausDistUB: #dimensions not supported"
		);
	}

	Region edge1 = Region(2);
	Region edge2 = Region(2);
	Region r = Region(2);

	double max = std::numeric_limits<double>::min();
	for (int i=0; i<4; i++) {
		this->getEdge(i,edge1);
		double min = std::numeric_limits<double>::max();
		for (int j=0; j<  vec_pShape.size(); j++) {
			vec_pShape[j]->getMBR(r);
			for (int k=0; k < 4; k++) {
				r.getEdge(k,edge2);
				min = std::min(min,edge1.getMaximumDistanceSq(edge2));
			}
		}

		max = std::max(max,min);
	}

	return std::sqrt(max);
}


/*
 * Auxilary methods
 *
 *
 */

double Region::getMinimumDistanceSq(const IShape& s) const
{
	const Region* pr = dynamic_cast<const Region*>(&s);
	if (pr != 0) return getMinimumDistanceSq(*pr);

	const Point* ppt = dynamic_cast<const Point*>(&s);
	if (ppt != 0) return getMinimumDistanceSq(*ppt);

	throw Tools::IllegalStateException(
		"Region::getHausDistUB: Not implemented yet!"
	);
}


double Region::getMinimumDistanceSq(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getMinimumDistance: Regions have different number of dimensions."
		);

	double ret = 0.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		double x = 0.0;

		if (r.m_pHigh[i] < m_pLow[i])
		{
			x = std::abs(r.m_pHigh[i] - m_pLow[i]);
		}
		else if (m_pHigh[i] < r.m_pLow[i])
		{
			x = std::abs(r.m_pLow[i] - m_pHigh[i]);
		}

		ret += x * x;
	}

	return ret;
}

double Region::getMinimumDistanceSq(const Point& p) const
{
	if (m_dimension != p.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getMinimumDistance: Point has different number of dimensions."
		);

	double ret = 0.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		if (p.getCoordinate(i) < m_pLow[i])
		{
			ret += std::pow(m_pLow[i] - p.getCoordinate(i), 2.0);
		}
		else if (p.getCoordinate(i) > m_pHigh[i])
		{
			ret += std::pow(p.getCoordinate(i) - m_pHigh[i], 2.0);
		}
	}

	return ret;
}


double Region::getMaximumDistanceSq(const Region& r) const
{
	if (m_dimension != r.m_dimension)
		throw Tools::IllegalArgumentException(
			"Region::getMinimumDistance: Regions have different number of dimensions."
		);

	double ret = 0.0;

	for (uint32_t i = 0; i < m_dimension; ++i)
	{
		double diff =
			std::max(
				std::abs(this->m_pLow[i] - r.m_pHigh[i]),
				std::abs(this->m_pHigh[i] - r.m_pLow[i])
			);

		ret += diff * diff;
	}

	return ret;

}



void Region::getEdge(int edgeId, Region& edge) const
{
	if (this->m_dimension != 2) {
		throw Tools::NotSupportedException(
			"Region::getEdge: #dimensions not supported"
		);
	}

	switch (edgeId) {
		case (0): // South Edge sw->se
			edge.m_pLow[1] = m_pLow[1]; // South
			edge.m_pLow[0] = m_pLow[0]; // West

			edge.m_pHigh[1] = m_pLow[1]; // South
			edge.m_pHigh[0] = m_pHigh[0]; // East
			break;


		case (1): // East Edge se->ne
			edge.m_pLow[1] = m_pLow[1]; // South
			edge.m_pLow[0] = m_pHigh[0]; // East

			edge.m_pHigh[1] = m_pHigh[1]; // North
			edge.m_pHigh[0] = m_pHigh[0]; // East
			break;


		case (2): // North Edge nw->ne
			edge.m_pLow[1] = m_pHigh[1]; // North
			edge.m_pLow[0] = m_pLow[0]; // West

			edge.m_pHigh[1] = m_pHigh[1]; // North
			edge.m_pHigh[0] = m_pHigh[0]; // East
			break;


		default: // West Edge sw->nw
			edge.m_pLow[1] = m_pLow[1]; // South
			edge.m_pLow[0] = m_pLow[0]; // West

			edge.m_pHigh[1] = m_pHigh[1]; // North
			edge.m_pHigh[0] = m_pLow[0]; // West
			break;
	}
}



