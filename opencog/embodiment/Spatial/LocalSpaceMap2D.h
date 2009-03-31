/*
 * opencog/embodiment/Spatial/LocalSpaceMap2D.h
 *
 * Copyright (C) 2007-2008 TO_COMPLETE
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef LOCAL_SPACE_MAP_2D_H
#define LOCAL_SPACE_MAP_2D_H

#include <math.h>

#include "util/Logger.h"
#include "util/numeric.h"
#include "util/functional.h"
#include "util/RandGen.h"

#include "LocalSpaceMap2DUtil.h"

#include <iostream>
#include <exception>
#include <boost/bind.hpp>
#include <string>

#include "SuperEntity.h"

/**
 * Represents a set of 2D object occuring within a given rectangle of space
 *
 * answers queries related to points, regions and trajectories within this
 * rectangle, based on a discretization (i.e. finite grid) of the region
 *
 * behavior is undefined for queries involving points outside this region
 */
namespace Spatial {

  class LocalSpaceMap2D;

  /**
   * Struct rec_find
   */
  struct rec_find {

    private:

    Spatial::GridPoint _current;
    const Spatial::GridPoint& _g;    

    const GridMap& _grid;

    Spatial::ObjectIDSet& _out;
            
    const Spatial::LocalSpaceMap2D* _parent;
    
    bool too_far() const;

    void move_left();
    void move_right();
    void move_up();
    void move_down();

    void rec();
    void check_grid();

    unsigned int step;
    unsigned int possible_step;
    unsigned int last_x_difference;
    unsigned int last_y_difference;

    public:
    rec_find(const Spatial::GridPoint& current, 
	     const Spatial::GridMap& grid,
	     Spatial::ObjectIDSet& out,
	     const Spatial::LocalSpaceMap2D& parent);

    void johnnie_walker(Spatial::Distance d);

  }; // struct rec_find

  class LocalSpaceMap2D {
    
  protected:
            
    Spatial::GridMap _grid;
    Spatial::GridMap _grid_nonObstacle;

//    ObjectHashMap objects;

    std::list<SuperEntityPtr> superEntities;
    LongEntityPtrHashMap entities;
    LongGridPointVectorHashMap gridPoints;

    bool addToSuperEntity( const EntityPtr& entity );

  private:
    Spatial::Distance _xMin;
    Spatial::Distance _xMax;
    unsigned int _xDim;

    Spatial::Distance _yMin;
    Spatial::Distance _yMax;
    unsigned int _yDim;

    Spatial::Distance _diagonalSize;
 
    //this is an argument to the constructor and stored so we can
    //cache things for performance
    Spatial::Distance _radius; 

    // used only for test
    const Spatial::GridSet _empty_set;

    bool outsideMap( const std::vector<Math::LineSegment>& segments );

    public:

    /**
     * Constructor
     */
    LocalSpaceMap2D(Distance xMin,Distance xMax,unsigned int xDim,
                    Distance yMin,Distance yMax,unsigned int yDim,
                    Distance radius);

    LocalSpaceMap2D( const LocalSpaceMap2D& map );

    LocalSpaceMap2D* clone() const;
    ~LocalSpaceMap2D();

    /**
     * Operator overloading
     */
    bool operator==(const LocalSpaceMap2D& other) const;

    /**
     * Persistence
     */
    void save(FILE* fp) const;
    void load(FILE* fp);
    void saveASCIIMap(const std::string fileName) const;

    Spatial::Distance xGridWidth() const;
    Spatial::Distance yGridWidth() const;

    Spatial::Distance xMin() const;
    Spatial::Distance xMax() const;
    unsigned int xDim() const;

    Spatial::Distance yMin() const;
    Spatial::Distance yMax() const;
    unsigned int yDim() const;

    Spatial::Distance diagonalSize() const; // just return a pre-calculated diagonal size

    Spatial::Distance radius() const;

    // Is the given point occupied by any obstacle or out of bounds?
    bool illegal(const Spatial::Point& pt) const;

    // Get the nearest free (not occupied or out of bounds) 
    // point from the given point 
    Spatial::Point getNearestFreePoint(const Spatial::Point& pt) const throw (opencog::RuntimeException, std::bad_exception);

    // Is the given point occupied by any obstacle or out of bounds?
    bool gridIllegal(const Spatial::GridPoint& gp) const;
    bool gridIllegal(unsigned int i, unsigned int j) const;

    bool gridOccupied(const Spatial::GridPoint& gp) const;
    bool gridOccupied(unsigned int i,unsigned int j) const;
           
    bool gridOccupied_nonObstacle(const Spatial::GridPoint& gp) const;
    bool gridOccupied_nonObstacle(unsigned int i,unsigned int j) const;

    //const ObjectMetaData& getMetaData(const Spatial::ObjectID& id) const throw(opencog::NotFoundException);

    bool containsObject(const Spatial::ObjectID& id) const;
    bool isObstacle(const Spatial::ObjectID& id) const;
    bool isNonObstacle(const Spatial::ObjectID& id) const;

    const std::vector<Spatial::GridPoint>& getObjectPoints(const Spatial::ObjectID& id) const throw(opencog::NotFoundException);

    // Note: At time of writing, this function is only used for the unit tests.
    // List all objects IDs (obstacles and non-obstacles) on this map
    template<typename Out>
    Out getAllObjects(Out out) const {

      ObjectIDSet localObjects;

      //ObjectHashMap::const_iterator it;
      LongEntityPtrHashMap::const_iterator it;
      //for( it = this->objects.begin( ); it != this->objects.end( ); ++it ) {
      for( it = this->entities.begin( ); it != this->entities.end( ); ++it ) {
	//localObjects.insert( it->first.c_str( ) );
	localObjects.insert( it->second->getName( ).c_str( ) );
      } // for

      return std::copy(localObjects.begin(), localObjects.end(), out);
    }

    // Copy all objects of the given map into this one
    void copyObjects(const LocalSpaceMap2D& otherMap);

    //remove an object from the map entirely
    void removeObject(const Spatial::ObjectID& id);

    //find the IDs of all objects within distance d of a certain point
    //same code use in findNearestFiltered
    template<typename Out>
    Out findEntities(const Spatial::GridPoint& g, Spatial::Distance d, Out out) const {
      Spatial::ObjectIDSet objs;
                
      struct rec_find finder(g, _grid, objs, *this);
      struct rec_find finderNonObstacle(g, _grid_nonObstacle, objs, *this);
                
      finder.johnnie_walker(d);
      finderNonObstacle.johnnie_walker(d);

      return std::copy(objs.begin(),objs.end(),out);
    
}
    //find the minimal distance between some point in an object and a reference
    //point
    Distance minDist(const Spatial::ObjectID& id, const Spatial::Point& p) const;

    //find the nearest entity to a given point satisfying some predicate
    template<typename Pred>
    ObjectID findNearestFiltered(const Point& p,Pred pred) const {

      std::vector<Spatial::ObjectID> tmp;
      Spatial::Distance d = 5;

      //convert to greidPoint
      Spatial::GridPoint g = snap(p);
                
      Distance x = fmax(xDim() - g.first -1, g.first);
      Distance y = fmax(yDim() - g.second -1, g.second);
      Spatial::Distance maxD = sqrt((x * x) + (y * y));
                
      Spatial::ObjectIDSet objs;
                
      struct rec_find finder(g, _grid, objs, *this);
      struct rec_find finderNonObstacle(g, _grid_nonObstacle, objs, *this);
                
      do {
	finder.johnnie_walker(d);
	finderNonObstacle.johnnie_walker(d);

	std::copy(objs.begin(),objs.end(), back_inserter(tmp));

	tmp.erase(std::partition(tmp.begin(), tmp.end(), pred), tmp.end());

	if (d >= maxD && tmp.empty()){
            opencog::logger().log(opencog::Logger::DEBUG, "LocalSpaceMap - Found no object that verifies the predicate. Distance %.2f, Max distance %.2f.", d, maxD); 
	  //won't find anything
	  return ObjectID();
	}
	d *= 1.5; //maybe it shouldn't grow so fast?
	if (d > maxD ){
	  d = maxD;
	}
	
      } while (tmp.empty());

      //now find the closest
      std::vector<Spatial::Distance> dists(tmp.size());
      std::transform(tmp.begin(), tmp.end(), dists.begin(),
		     boost::bind(&LocalSpaceMap2D::minDist, this, _1,boost::cref(p)));

      return *(tmp.begin() + distance(dists.begin(), 
				      min_element(dists.begin(), dists.end())));
    }

    //find a random entity satisfying some predicate
    template<typename Pred>
    ObjectID findRandomFiltered(Pred pred, opencog::RandGen& rng) const {

      //filter out all entities that match
      std::vector<Spatial::ObjectID> tmp;

      //ObjectHashMap::const_iterator it;
      LongEntityPtrHashMap::const_iterator it;
      //for( it = this->objects.begin( ); it != this->objects.end( ); ++it ) {
      for( it = this->entities.begin( ); it != this->entities.end( ); ++it ) {
	//if ( pred( it->first ) ){
	if ( pred( it->second->getName( ) ) ){
	  //tmp.push_back( it->first );
	  tmp.push_back( it->second->getName( ) );
	} // if
      } // for

      return tmp.empty() ? ObjectID() : tmp[rng.randint(tmp.size())];
    }

    // return id's of all objects within the space map
    template<typename Out>
    Out findAllEntities(Out out) const {
                
      // If one object id is stored as an obstacle and a non-obstacle,
      // only count it once:
      Spatial::ObjectIDSet localObjects;

      //ObjectHashMap::const_iterator it;
      LongEntityPtrHashMap::const_iterator it;
      //for( it = this->objects.begin( ); it != this->objects.end( ); ++it ) {
      for( it = this->entities.begin( ); it != this->entities.end( ); ++it ) {
	//localObjects.insert( it->first.c_str( ) );
	localObjects.insert( it->second->getName( ).c_str( ) );
      } // for

      return std::copy(localObjects.begin(), localObjects.end(), out);
    }

    //all of the points associated with a given object
    template<typename Out>
    Out allPoints(const Spatial::ObjectID& id, Out out) const {
      std::vector<Spatial::Point> points;

      //ObjectHashMap::const_iterator it = this->objects.find( id );
      long idHash = boost::hash<std::string>()( id );
      LongGridPointVectorHashMap::const_iterator it = this->gridPoints.find( idHash );
      //if ( it != this->objects.end( ) ) {
      if ( it != this->gridPoints.end( ) ) {
	unsigned int i;
	//for( i = 0; i < it->second.gridPoints.size( ); ++i ) {
	for( i = 0; i < it->second.size( ); ++i ) {
	  //points.push_back( unsnap( it->second.gridPoints[ i ] ) );
	  points.push_back( unsnap( it->second[ i ] ) );
	} // for
      } // if

      return std::copy(points.begin(), points.end(), out);
    }

    //Euclidean distance between points
    template<typename PointT>
    static Distance eucDist(const PointT& p1,const PointT& p2) {
      return std::sqrt(opencog::power(p1.first - p2.first, 2)+
		       opencog::power(p1.second - p2.second, 2));
    }

    //are the given /grid coordinates/ within the dimensions of this grid?
    bool coordinatesAreOnGrid(unsigned int x, unsigned int y) const;

    //should return a nearby (to the src) unoccupied point (taking _radius into
    //account) on the edge of the object with the given id currently just
    //returns the center
    Spatial::Point nearbyPoint(const Spatial::Point& src, const Spatial::ObjectID& id) const;

    //should return an unoccupied point p (taking _radius into account) on or
    //near the edge of the object 'id' such that the object falls between src
    //and p
    //
    //formally, behindPoint returns a point x such that p, centerOf(id), and x
    //are approximately collinear, centerOf(id) is between p and x, x is
    //unoccupied (taking _radius into account), and the euclidean distance
    //between centerOf(id) and x is minimized
    Spatial::Point behindPoint(const Spatial::Point& src, const Spatial::ObjectID& id) const;

    //returns the first free point traveling from src in direction if no free
    //point is found, will return the last valid point encountered
    Spatial::Point findFree(const Spatial::Point& p, const Spatial::Point& direction) const;

    //the center of a object - currently this is computed as the center of the
    //bounding rectangle, but e.g. and average would be fine to - not sure
    //which will give nicer results in SL or if it will matter
    Spatial::Point centerOf(const Spatial::ObjectID& id) const;

    //nearest point on grid
    Spatial::GridPoint snap(const Spatial::Point& p) const;

    //back to the original coord system
    Spatial::Point unsnap(const Spatial::GridPoint& g) const;

    /**
     * Find a free point near a given position, at a given distance
     * @param position Given position
     * @param distance Maximum distance from the given position to search the free point
     * @param startDirection Vector that points to the direction of the first rayTrace
     */      
    Spatial::Point getNearFreePointAtDistance( const Spatial::Point& position, float distance, const Spatial::Point& startDirection ) const throw (opencog::NotFoundException);
      
    /**
     * Get the nearest object point of a given a reference point
     *
     * @param referencePoint Point used as a reference to find the nearest object point
     * @param objectID Object ID
     * @return The nearest object point
     */
    Spatial::Point getNearestObjectPoint( const Spatial::Point& referencePoint, const Spatial::ObjectID& objectID ) const throw(opencog::NotFoundException);

    /**
     * Find all points belonging to a given line segment
     *
     * @param points Calculated vector grid points (out)
     * @param segment Segment used to calculate the points
     */
    void calculateSegmentGridPoints( std::vector<Spatial::GridPoint>& points, const Math::LineSegment& segment );      

    /**
     * Find all points belonging to an object, represented by it's segments
     * 
     * @param points Calculated vector grid points (out)
     * @param segments Vector of segments that represtends an object
     */
    void calculateObjectPoints( std::vector<Spatial::GridPoint>& points, const std::vector<Math::LineSegment>& segments );

    /**
     * Add an object to the map
     *
     * @param id Object id
     * @param metadata Object metadata
     * @param isObstacle If true the object will be considered an obstacle, false an nonObstacle
     */
    void addObject( const Spatial::ObjectID& id, const Spatial::ObjectMetaData& metadata, bool isObstacle = false );

    /**
     * Update the points of an object. If the object metadata has not changed, 
     * nothing will be done. If it was changed from object<->nonObject it will be updated too.
     *
     * @param id Object id
     * @param metadata Object metadata
     * @param isObstacle If true the object will be considered an obstacle, false an nonObstacle
     */
    void updateObject( const Spatial::ObjectID& id, const Spatial::ObjectMetaData& metadata, bool isObstacle = false );
    
    //const Object& getObject( const Spatial::ObjectID& id ) const throw (opencog::NotFoundException);

    const EntityPtr& getEntity( const std::string& id ) const throw (opencog::NotFoundException);
    const EntityPtr& getEntity( long id ) const throw (opencog::NotFoundException);
    
    bool belongsToSuperEntity( const Spatial::ObjectID& id ) const;
    
    const SuperEntityPtr& getSuperEntityWhichContains( const Spatial::ObjectID& id ) const throw(opencog::NotFoundException);

    inline const std::list<SuperEntityPtr>& getSuperEntities( void ) const {
      return this->superEntities;
    }


  }; // struct LocalSpaceMap2D
   
} //~namespace Spatial

#endif
