#include <modelling_input.h>
#include <modelling_parameters.h>
#include <modeling_methods.h>
#include <continuous_property.h>
#include <math_methods.h>

#include <algorithm>
#include <functional>

using namespace std;

bool Basic_input::get_interface_data()
{
	if ((int)itrface->size() < 2) return false;

	_get_distinct_interface_iso_values();
	_get_interface_points();

	return true;
}

void Basic_input::_get_distinct_interface_iso_values()
{
	std::vector <double> iso_values;
	iso_values.push_back(itrface->at(0).level());
	for (int j = 1; j <(int)itrface->size(); j++){
		// search existing list of iso values
		int add = 0;
		for (int k = 0; k < (int)iso_values.size(); k++ ){
			if (itrface->at(j).level() != iso_values[k]) add++;
		}
		if (add == (int)iso_values.size()) // this is a iso value not in the list yet
		{
			// add new iso value to list
			iso_values.push_back(itrface->at(j).level());
		}
	}

	// sort the vector (largest to smallest) - done for convience and for functional reasons 
	std::sort(iso_values.begin(), iso_values.end(), std::greater<double>());

	for (int j = 0; j <(int)iso_values.size(); j++ ) interface_iso_values->push_back(iso_values[j]);
}

void Basic_input::_get_interface_points()
{
	// interface[0][0,1,2,3,....] points 0,1,2,3,.... belong to the 0th interface 
	// ...
	// interface[m = interface_iso_values.size()][76,45,43,4,.....] points 76,45,43,4,..... belong to the mth interface
	interface_point_lists->resize((int)interface_iso_values->size());
	for (int j = 0; j < (int)interface_iso_values->size(); j++ ){
		for (int k = 0; k < (int)itrface->size(); k++ ){
			if (itrface->at(k).level() == interface_iso_values->at(j) )
			{
				// add to 2D vector 
				interface_point_lists->at(j).push_back(itrface->at(k));
			}
		}
	}

	for (int j = 0; j < (int)interface_point_lists->size(); j++ ){
		// set the test_interface_points 
		interface_test_points->push_back(interface_point_lists->at(j)[0]);
		if ((int)interface_point_lists->at(j).size() == 1)
		{
			// need to have at least 2 points per interface
			// remove this interface from the list
			interface_point_lists->erase(interface_point_lists->begin() + j);
			j--;
		}
	}
}

bool Planar::_compute_strike_dip_polarity_from_normal()
{
	if ( _normal[2] < 0 ) _polarity = 1;
	else _polarity = 0;

	double r2d = 57.295779513082320876798154814105;
	// get dip first 
	_dip = acos(_normal[2])*r2d; // could do better. e.g. for overturn cases puts _dip > 90. but there formula's for getting normals works so sticking with it for now
	// get dip_direction
	double dip_direction = atan2(_normal[1],_normal[0])*r2d;


	// if negative azimuth get positive angle
	if (dip_direction < 0) dip_direction += 360;

	// get strike
	_strike = 360 - dip_direction;

	return true; // check this computation
}

bool Planar::_compute_normal_from_strike_dip_polarity()
{
	double d2r = 0.01745329251994329576923690768489;

	// Get down dip vector - v
	double vx = cos(-1.0*(_strike * d2r)) * cos(-1.0*(_dip * d2r));
	double vy = sin(-1.0*(_strike * d2r)) * cos(-1.0*(_dip * d2r));
	double vz = sin(-1.0*(_dip * d2r));

	// Get strike vector - vp
	double vpx = -1.0 * vy;
	double vpy = vx;
	double vpz = 0; 

	// Compute cross product of v x vp
	double Nx = (vy * vpz) - (vz * vpy); 
	double Ny = (vz * vpx) - (vx * vpz);
	double Nz = (vx * vpy) - (vy * vpx);

	// Compute Length
	double Length = sqrt(Nx*Nx + Ny*Ny + Nz*Nz);

	// Normalize
	Nx = Nx / Length;
	Ny = Ny / Length;
	Nz = Nz / Length;

	// Check polarity of computed normal is consistent with the property "polarity"
	// polarity == 1 -> Is overturned: Must point downward
	// polarity == 0 -> Is upright: Must point upward
	// polarity != 0/1 -> Is unknown: Can point upward or downward, it doesn't matter.
	//                    Retains its initialized polarity set above.
	if ((_polarity == 1 && Nz > 0) || (_polarity == 0 && Nz < 0) )
	{
		// Flip vector
		Nx = -Nx;
		Ny = -Ny;
		Nz = -Nz;
	}

	// assign normal
	_normal[0] = Nx;
	_normal[1] = Ny;
	_normal[2] = Nz;

	return true;
}

bool Planar::getDipVector( double (&vector)[3] )
{
	double d2r = 0.01745329251994329576923690768489;

	// Get down dip vector - v
	double vx = cos(-1.0*(_strike * d2r)) * cos(-1.0*(_dip * d2r));
	double vy = sin(-1.0*(_strike * d2r)) * cos(-1.0*(_dip * d2r));
	double vz = sin(-1.0*(_dip * d2r));

	// normalize
	double length = sqrt(vx*vx + vy*vy + vz*vz);
	vx /= length;
	vy /= length;
	vz /= length;

	vector[0] = vx;
	vector[1] = vy;
	vector[2] = vz;

	return true;
}

bool Planar::getStrikeVector( double (&vector)[3] )
{
	double d2r = 0.01745329251994329576923690768489;

	// Get strike vector
	double vx = -sin(-1.0*(_strike * d2r));
	double vy = cos(-1.0*(_strike * d2r));
	double vz = 0;

	vector[0] = vx;
	vector[1] = vy;
	vector[2] = vz;

	return true;
}

bool Planar::getNormalError( double (&matrix)[3][2] )
{
	double d2r = 0.01745329251994329576923690768489;

	double r = 10.0*d2r; // err
	double d = _dip*d2r;
	double s = _strike*d2r;
	double d1x = (cos(d + r/2.)*cos(r + s)*sin(d + r/2.))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r + s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2),2));
	double d1y =  -((cos(d + r/2.)*sin(d + r/2.)*sin(r + s))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r + s),2)*pow(sin(d + r/2.),2) + 
		pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2),2)));
	double d1z =  (pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r + s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2),2));
	if ( _polarity == 1 )
	{
		d1x*=-1.0;
		d1y*=-1.0;
		d1z*=-1.0;
	}

	double d2x = (cos(d - r/2.)*cos(r - s)*sin(d - r/2.))/
		sqrt(pow(cos(d - r/2.),2)*pow(cos(r - s),2)*pow(sin(d - r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d - r/2.),2)*pow(sin(r - s),2) + 
		pow(pow(cos(d - r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2),2));
	double d2y = (cos(d + r/2.)*sin(d - r/2.)*sin(r - s))/
		sqrt(pow(cos(d - r/2.),2)*pow(cos(r - s),2)*pow(sin(d - r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d - r/2.),2)*pow(sin(r - s),2) + 
		pow(pow(cos(d - r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2),2));
	double d2z = (pow(cos(d - r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2))/
		sqrt(pow(cos(d - r/2.),2)*pow(cos(r - s),2)*pow(sin(d - r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d - r/2.),2)*pow(sin(r - s),2) + 
		pow(pow(cos(d - r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2),2));
	if ( _polarity == 1 )
	{
		d2x*=-1.0;
		d2y*=-1.0;
		d2z*=-1.0;
	}

	double d3x = (cos(d - r/2.)*cos(r + s)*sin(d - r/2.))/
		sqrt(pow(cos(d - r/2.),2)*pow(cos(r + s),2)*pow(sin(d - r/2.),2) + pow(cos(d - r/2.),2)*pow(sin(d - r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d - r/2.),2)*pow(cos(r + s),2) + pow(cos(d - r/2.),2)*pow(sin(r + s),2),2));
	double d3y = -((cos(d - r/2.)*sin(d - r/2.)*sin(r + s))/
		sqrt(pow(cos(d - r/2.),2)*pow(cos(r + s),2)*pow(sin(d - r/2.),2) + 
		pow(cos(d - r/2.),2)*pow(sin(d - r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d - r/2.),2)*pow(cos(r + s),2) + pow(cos(d - r/2.),2)*pow(sin(r + s),2),2)));
	double d3z = (pow(cos(d - r/2.),2)*pow(cos(r + s),2) + pow(cos(d - r/2.),2)*pow(sin(r + s),2))/
		sqrt(pow(cos(d - r/2.),2)*pow(cos(r + s),2)*pow(sin(d - r/2.),2) + pow(cos(d - r/2.),2)*pow(sin(d - r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d - r/2.),2)*pow(cos(r + s),2) + pow(cos(d - r/2.),2)*pow(sin(r + s),2),2));
	if ( _polarity == 1 )
	{
		d3x*=-1.0;
		d3y*=-1.0;
		d3z*=-1.0;
	}

	double d4x = (cos(d + r/2.)*cos(r - s)*sin(d + r/2.))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r - s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r - s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2),2));
	double d4y = (cos(d + r/2.)*sin(d + r/2.)*sin(r - s))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r - s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r - s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2),2));
	double d4z = (pow(cos(d + r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r - s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r - s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r - s),2) + pow(cos(d + r/2.),2)*pow(sin(r - s),2),2));
	if ( _polarity == 1 )
	{
		d4x*=-1.0;
		d4y*=-1.0;
		d4z*=-1.0;
	}

	double nx_l = d1x;
	if ( d2x < nx_l) nx_l = d2x;
	if ( d3x < nx_l) nx_l = d3x;
	if ( d4x < nx_l) nx_l = d4x;
	double ny_l = d1y;
	if ( d2y < ny_l) ny_l = d2y;
	if ( d3y < ny_l) ny_l = d3y;
	if ( d4y < ny_l) ny_l = d4y;
	double nz_l = d1z;
	if ( d2z < nz_l) nz_l = d2z;
	if ( d3z < nz_l) nz_l = d3z;
	if ( d4z < nz_l) nz_l = d4z;

	double nx_u = d1x;
	if ( d2x > nx_u) nx_u = d2x;
	if ( d3x > nx_u) nx_u = d3x;
	if ( d4x > nx_u) nx_u = d4x;
	double ny_u = d1y;
	if ( d2y > ny_u) ny_u = d2y;
	if ( d3y > ny_u) ny_u = d3y;
	if ( d4y > ny_u) ny_u = d4y;
	double nz_u = d1z;
	if ( d2z > nz_u) nz_u = d2z;
	if ( d3z > nz_u) nz_u = d3z;
	if ( d4z > nz_u) nz_u = d4z;

	matrix[0][0] = nx_l;
	matrix[0][1] = nx_u;
	matrix[1][0] = ny_l;
	matrix[1][1] = ny_u;
	matrix[2][0] = nz_l;
	matrix[2][1] = nz_u;

	r = 0;
	double nnx = (cos(d + r/2.)*cos(r + s)*sin(d + r/2.))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r + s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2),2));
	double nny = -((cos(d + r/2.)*sin(d + r/2.)*sin(r + s))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r + s),2)*pow(sin(d + r/2.),2) + 
		pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2),2)));
	double nnz =  (pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2))/
		sqrt(pow(cos(d + r/2.),2)*pow(cos(r + s),2)*pow(sin(d + r/2.),2) + pow(cos(d + r/2.),2)*pow(sin(d + r/2.),2)*pow(sin(r + s),2) + 
		pow(pow(cos(d + r/2.),2)*pow(cos(r + s),2) + pow(cos(d + r/2.),2)*pow(sin(r + s),2),2));
	if ( _polarity == 1 )
	{
		nnx*=-1.0;
		nny*=-1.0;
		nnz*=-1.0;
	}

	double nx = _normal[0];
	double ny = _normal[1];
	double nz = _normal[2];

	return true;
}

double distance_btw_pts( const Point &p1, const Point &p2 )
{
	double dx = p1.x() - p2.x();
	double dy = p1.y() - p2.y();
	double dz = p1.z() - p2.z();
	double dc = p1.c() - p2.c();

	return sqrt(dx*dx + dy*dy + dz*dz + dc*dc);
}

int nearest_neighbour_index( const Point &p, const std::vector < Point > &pts )
{
	// following is a bit more elaborate than it could be 
	// because p could be in the list pts[]. In that case
	// you will want to have the next nearest neighbour

	// calculate all the distances + create index array
	std::vector < double > dist_arr;
	std::vector < int > index_arr;
	for (int j = 0; j < (int)pts.size(); j++ ){
		dist_arr.push_back( distance_btw_pts(p, pts[j]));
		index_arr.push_back(j);
	}
	// sort distances and corresponding indicies 
	Math_methods::sort_vector_w_index(dist_arr,index_arr); // smallest to largest

	for (int j = 0; j < (int)pts.size(); j++ ){
		if ( dist_arr[j] != 0 ) return index_arr[j];
	}
	return -1;
}

int furtherest_neighbour_index( const Point &p, const std::vector < Point > &pts )
{
	int index = 0;
	double largest_distance = distance_btw_pts(p, pts[0]);
	for (int j = 1; j < (int)pts.size(); j++ ){
		double distance_j = distance_btw_pts(p,pts[j]);
		if ( distance_j > largest_distance )
		{
			largest_distance = distance_j;
			index = j;
		}
	}
	return index;
}

int furtherest_neighbour_index( const std::vector < Point > &pts1, const std::vector < Point > &pts2 )
{
	// find a point in PTS1 dataset that is furthest away from PTS2 dataset

	int index = 0;
	double largest_distance = 0.0;
	for (int j = 0; j < (int)pts1.size(); j++ ){
		for (int k = 0; k < (int)pts2.size(); k++ ){
			double distance_jk = distance_btw_pts(pts1[j],pts2[k]);
			if ( distance_jk > largest_distance )
			{
				largest_distance = distance_jk;
				index = j;
			}
		}
	}
	return index;
}

bool Find_STL_Vector_Indices_FurtherestTwoPoints(const std::vector< Point> &pts, int(&TwoIndexes)[2])
{
	if (pts.size() < 2) return false;

	double largest_distance = 0.0;
	for (int j = 0; j < (int)pts.size(); j++){
		for (int k = 0; k < (int)pts.size(); k++){
			double distance_jk = distance_btw_pts(pts[j], pts[k]);
			if (distance_jk > largest_distance)
			{
				largest_distance = distance_jk;
				TwoIndexes[0] = j;
				TwoIndexes[1] = k;
			}
		}
	}

	return true;
}

int Find_STL_Vector_Index_ofPointClosestToOtherPointWithinDistance(const Point &p, const std::vector< Point > &pts, const double &dist)
{
	double smallest_residual = 100000000000;
	int index = -1;
	for (int j = 0; j < (int)pts.size(); j++){
		double distance = distance_btw_pts(p, pts[j]);
		double residual = abs(distance - dist);
		if (residual < smallest_residual)
		{
			smallest_residual = residual;
			index = j;
		}
	}

	return index;
}

void calculate_bounds( const std::vector< Point > &pts, double (&bounds)[6] )
{
	// bounds[6] = { xmin, xmax, ymin, ymax, zmin, zmax }
	bounds[0] = pts[0].x();
	bounds[1] = pts[0].x();
	bounds[2] = pts[0].y();
	bounds[3] = pts[0].y();
	bounds[4] = pts[0].z();
	bounds[5] = pts[0].z();

	for (int j = 1; j < (int)pts.size(); j++ ){
		if ( pts[j].x() < bounds[0] ) bounds[0] = pts[j].x(); // x-min
		if ( pts[j].x() > bounds[1] ) bounds[1] = pts[j].x(); // x-max 
		if ( pts[j].y() < bounds[2] ) bounds[2] = pts[j].y(); // y-min 
		if ( pts[j].y() > bounds[3] ) bounds[3] = pts[j].y(); // y-max 
		if ( pts[j].z() < bounds[4] ) bounds[4] = pts[j].z(); // z-min 
		if ( pts[j].z() > bounds[5] ) bounds[5] = pts[j].z(); // z-max
	}
}

std::vector< int > get_extremal_point_data_indices_from_points( const std::vector< Point > &pts)
{
	// return a vector of indices from pts[] that maximally samples the data space
	int n = (int)pts.size();

	// sort x-coord, y-coord, and z-coord data and corresponding indices
	std::vector< double > x_coord;
	std::vector< double > y_coord;
	std::vector< double > z_coord;
	std::vector< int > x_idx;
	std::vector< int > y_idx;
	std::vector< int > z_idx;
	for (int j = 0; j < n; j++ ){
		x_coord.push_back(pts[j].x());
		y_coord.push_back(pts[j].y());
		z_coord.push_back(pts[j].z());
		x_idx.push_back(j);
		y_idx.push_back(j);
		z_idx.push_back(j);
	}
	Math_methods::sort_vector_w_index(x_coord,x_idx);
	Math_methods::sort_vector_w_index(y_coord,y_idx);
	Math_methods::sort_vector_w_index(z_coord,z_idx);


	// compute the order of axial sampling variability, largest to smallest
	std::vector < double > ranges;
	std::vector < int > axis;
	ranges.push_back( x_coord[n - 1] - x_coord[0] ); // x-range
	ranges.push_back( y_coord[n - 1] - y_coord[0] ); // y-range
	ranges.push_back( z_coord[n - 1] - z_coord[0] ); // z-range
	for (int j = 0; j < 3; j++ ) axis.push_back(j); // x-axis = 0, y-axis = 1, z-axis = 2 
	Math_methods::sort_vector_w_index(ranges, axis); // sort smallest to largest
	int axis_order[3] = {axis[2], axis[1], axis[0]}; // re-arrange largest to smallest

	std::vector < int > data_indices; // result container
	for (int j = 0; j < 3; j++ ){
		if ( axis_order[j] == 0 ) // x-axis
		{
			if (!is_index_in_list(x_idx[0], data_indices)) data_indices.push_back(x_idx[0]);
			else
			{
				for (int k = 1; k < n; k++ )
				{
					if (!is_index_in_list(x_idx[k], data_indices))
					{
						data_indices.push_back(x_idx[k]);
						break;
					}
				}
			}

			if (!is_index_in_list(x_idx[n - 1], data_indices)) data_indices.push_back(x_idx[n - 1]);
			else
			{
				for (int k = 1; k < n; k++ )
				{
					if (!is_index_in_list(x_idx[n - k - 1], data_indices))
					{
						data_indices.push_back(x_idx[n - k - 1]);
						break;
					}
				}
			}
		}
		if ( axis_order[j] == 1 ) // y-axis
		{
			if (!is_index_in_list(y_idx[0], data_indices)) data_indices.push_back(y_idx[0]);
			else
			{
				for (int k = 1; k < n; k++ )
				{
					if (!is_index_in_list(y_idx[k], data_indices))
					{
						data_indices.push_back(y_idx[k]);
						break;
					}
				}
			}

			if (!is_index_in_list(y_idx[n - 1], data_indices)) data_indices.push_back(y_idx[n - 1]);
			else
			{
				for (int k = 1; k < n; k++ )
				{
					if (!is_index_in_list(y_idx[n - k - 1], data_indices))
					{
						data_indices.push_back(y_idx[n - k - 1]);
						break;
					}
				}
			}
		}
		if ( axis_order[j] == 2 ) // z-axis
		{
			if (!is_index_in_list(z_idx[0], data_indices)) data_indices.push_back(z_idx[0]);
			else
			{
				for (int k = 1; k < n; k++ )
				{
					if (!is_index_in_list(z_idx[k], data_indices))
					{
						data_indices.push_back(z_idx[k]);
						break;
					}
				}
			}

			if (!is_index_in_list(z_idx[n - 1], data_indices)) data_indices.push_back(z_idx[n - 1]);
			else
			{
				for (int k = 1; k < n; k++ )
				{
					if (!is_index_in_list(z_idx[n - k - 1], data_indices))
					{
						data_indices.push_back(z_idx[n - k - 1]);
						break;
					}
				}
			}
		}
	}

	return data_indices;
}

bool get_maximal_axial_variability_order( const double(&bounds)[6], Parameter_Types::AXIS (&axis_order)[3] )
{
	std::vector < double > ranges;
	std::vector < int > axis;
	for (int j = 0; j < 3; j++ ){
		ranges.push_back( abs( bounds[2*j] - bounds[2*j + 1]) );
		axis.push_back(j);
	}
	Math_methods::sort_vector_w_index(ranges, axis);

	int idx = 0;
	for (int j = 2; j >= 0; j-- ){
		if (axis[j] == 0) axis_order[idx] = Parameter_Types::AXIS::Xaxis;
		if (axis[j] == 1) axis_order[idx] = Parameter_Types::AXIS::Yaxis;
		if (axis[j] == 2) axis_order[idx] = Parameter_Types::AXIS::Zaxis;
		idx++;
	}

	return true;
}

bool is_index_in_list( const int &index, const std::vector < int > &list )
{
	for (int j = 0; j < (int)list.size(); j++ ){
		if (list[j] == index) return true;
	}
	return false;
}

bool find_fill_distance( const Basic_input &input, double &fill_distance )
{
	std::vector< double > ndist_j;

	// put all inputted constraints into a vector of Points
	std::vector < Point > points;

	for (int j = 0; j < (int)input.inequality->size(); j++ ) points.push_back(input.inequality->at(j));
	for (int j = 0; j < (int)input.itrface->size(); j++) points.push_back(input.itrface->at(j));
	for (int j = 0; j < (int)input.planar->size(); j++) points.push_back(input.planar->at(j));
	for (int j = 0; j < (int)input.tangent->size(); j++) points.push_back(input.tangent->at(j));

	if (points.size() == 0 || input.evaluation_pts->size() == 0) return false;

	for (int j = 0; j < (int)input.evaluation_pts->size(); j++ ){
		// find closest point in points[] to evaluation_pts[j]
		unsigned int index = nearest_neighbour_index(input.evaluation_pts->at(j), points);
		// compute nearest neighbour distance and push into ndist_j
		ndist_j.push_back(distance_btw_pts(input.evaluation_pts->at(j), points[index]));
	}

	// find largest element in ndist_j
	double largest = ndist_j[0];
	for (int j = 1; j < (int)ndist_j.size(); j++ ) if ( ndist_j[j] > largest ) largest = ndist_j[j];

	fill_distance = largest;
	return true;
}

std::vector<int> Get_Inequality_STL_Vector_Indices_With_Large_Residuals( const std::vector<Inequality> *inequality, const double &avg_nn_distance )
{
	// Function will intelligently* get the indices within the STL vector of Inequality points that have large residuals
	// Intelligently* : Doesn't blindly capture all points with large residuals 
	//                 - Considers the distance to other large residual points
	//               These are a bit special
	std::vector<int> inequality_indices_to_include; // what we are going to function on function exit

	double Large_distance = 1000000000;

	std::vector < int > inequality_residuals_indices;
	for (int j = 0; j < (int)inequality->size(); j++ ){
		if ( !inequality->at(j).residual() ) inequality_residuals_indices.push_back(j);
	}
	if ( inequality_residuals_indices.size() != 0)
	{
		// Will always accept the first residual over the threshold 
		inequality_indices_to_include.push_back(inequality_residuals_indices[0]);
		inequality_residuals_indices.pop_back();

		for (int j = 0; j < (int)inequality_residuals_indices.size(); j++ ){
			// current index being analyzed (next residual in list): 
			int index = inequality_residuals_indices[j];
			// find closest point currently in inequality_indices_to_include[] to index
			double nn_dist = Large_distance;
			int nn_index = -1; // should always be overwritten. Will get a seg fault if this isn't the case
			for (int k = 0; k < (int)inequality_indices_to_include.size(); k++ ){
				double dist = distance_btw_pts(inequality->at(index),inequality->at(inequality_indices_to_include[k]));
				if ( dist < nn_dist )
				{
					nn_dist = dist;
					nn_index = inequality_indices_to_include[k];
				}
			}
			// Distance to other Large Residual Points Condition
			if ( nn_dist  > avg_nn_distance) inequality_indices_to_include.push_back(index);
		}
	}

	return inequality_indices_to_include;
}

std::vector<int> Get_Interface_STL_Vector_Indices_With_Large_Residuals( 
	const std::vector<Interface> *itrface, 
	const double &itrface_uncertainty, 
	const double &avg_nn_distance  )
{
	// Function will intelligently* get the indices within the STL vector of Interface points that have large residuals
	// Intelligently* : Doesn't blindly capture all points with large residuals 
	//                 - Considers the magnitude of the residuals
	//                 - Considers the distance to other large residual points
	//                 - Considers the variability with close large residual points

	std::vector<int> itrface_indices_to_include; // what we are going to function on function exit

	double Large_distance = 1000000000;

	std::vector < double > large_itrface_residuals;
	std::vector < int > large_itrface_residuals_indices;
	for (int j = 0; j < (int)itrface->size(); j++ ){
		double error = itrface->at(j).residual();
		if ( error > itrface_uncertainty )
		{
			large_itrface_residuals.push_back(error);
			large_itrface_residuals_indices.push_back(j);
		}
	}
	if ( large_itrface_residuals.size() != 0)
	{
		// Residual Magnitude Condition
		// sort all residuals over the threshold(angular_uncertainty) in smallest to largest - along with linked indices
		Math_methods::sort_vector_w_index(large_itrface_residuals,large_itrface_residuals_indices);
		// Will always accept largest residual over the threshold 
		itrface_indices_to_include.push_back(large_itrface_residuals_indices[(int)large_itrface_residuals_indices.size() - 1]);
		large_itrface_residuals.pop_back(); // probably don't need to do this since we never use it again
		large_itrface_residuals_indices.pop_back();

		for (int j = 0; j < (int)large_itrface_residuals_indices.size(); j++ ){
			// current index being analyzed (next largest residual in list): 
			int index = large_itrface_residuals_indices[(int)large_itrface_residuals_indices.size()- j - 1];
			// find closest point currently in itrface_indices_to_include[] to index
			double nn_dist = Large_distance;
			int nn_index = -1; // should always be overwritten. Will get a seg fault if this isn't the case
			for (int k = 0; k < (int)itrface_indices_to_include.size(); k++ ){
				double dist = distance_btw_pts(itrface->at(index),itrface->at(itrface_indices_to_include[k]));
				if ( dist < nn_dist )
				{
					nn_dist = dist;
					nn_index = itrface_indices_to_include[k];
				}
			}
			// Distance to other Large Residual Points Condition
			if ( nn_dist  > avg_nn_distance) itrface_indices_to_include.push_back(index);
			else
			{
				// Variability Condition
				// sometimes there are points closely together that exhibit a large amount of variability 
				// determine if that is the care - if so, include point
				// compare measurement of index to nn_index
				// using the measurement of the scalar_field() - this assumes that GRBF_Modelling_Methods->measure_residuals()
				// has been called already 
				double variability = abs(itrface->at(index).scalar_field() - itrface->at(nn_index).scalar_field() );
				if ( variability > itrface_uncertainty ) itrface_indices_to_include.push_back(index);
 			}
		}
	}

	return itrface_indices_to_include;
}

std::vector<int> Get_Planar_STL_Vector_Indices_With_Large_Residuals( const std::vector<Planar> *planar, const double &angular_uncertainty, const double &avg_nn_distance )
{
	// Function will intelligently* get the indices within the STL vector of Planar points that have large residuals
	// Intelligently* : Doesn't blindly capture all points with large residuals 
	//                 - Considers the magnitude of the residuals
	//                 - Considers the distance to other large residual points
	//                 - Considers the variability with close large residual points

	std::vector<int> planar_indices_to_include; // what we are going to function on function exit

	double r2d = 57.29577951308232;
	double Large_distance = 1000000000;

	std::vector < double > large_planar_residuals;
	std::vector < int > large_planar_residuals_indices;
	for (int j = 0; j < (int)planar->size(); j++ ){
		double grad_err = planar->at(j).residual()*r2d;
		if ( grad_err > angular_uncertainty )
		{
			large_planar_residuals.push_back(grad_err);
			large_planar_residuals_indices.push_back(j);
		}
	}
	if ( large_planar_residuals.size() != 0)
	{
		// Residual Magnitude Condition
		// sort all residuals over the threshold(angular_uncertainty) in smallest to largest - along with linked indices
		Math_methods::sort_vector_w_index(large_planar_residuals,large_planar_residuals_indices);
		// Will always accept largest residual over the threshold 
		planar_indices_to_include.push_back(large_planar_residuals_indices[(int)large_planar_residuals_indices.size() - 1]);
		large_planar_residuals.pop_back(); // probably don't need to do this since we never use it again
		large_planar_residuals_indices.pop_back();

		for (int j = 0; j < (int)large_planar_residuals_indices.size(); j++ ){
			// current index being analyzed (next largest residual in list): 
			int index = large_planar_residuals_indices[(int)large_planar_residuals_indices.size()- j - 1];
			// find closest point currently in planar_indices_to_include[] to index
			double nn_dist = Large_distance;
			int nn_index = -1; // should always be overwritten. Will get a seg fault if this isn't the case
			for (int k = 0; k < (int)planar_indices_to_include.size(); k++ ){
				double dist = distance_btw_pts(planar->at(index),planar->at(planar_indices_to_include[k]));
				if ( dist < nn_dist )
				{
					nn_dist = dist;
					nn_index = planar_indices_to_include[k];
				}
			}
			// Distance to other Large Residual Points Condition
			if ( nn_dist  > avg_nn_distance) planar_indices_to_include.push_back(index);
			else
			{
				// Variability Condition
				// sometimes there are points closely together that exhibit a large amount of variability 
				// determine if that is the care - if so, include point
				// compare measurement of index to nn_index
				double angle = 0.0;
				std::vector<double> v1;
				std::vector<double> v2;
				v1.push_back(planar->at(index).nx());
				v1.push_back(planar->at(index).ny());
				v1.push_back(planar->at(index).nz());
				v2.push_back(planar->at(nn_index).nx());
				v2.push_back(planar->at(nn_index).ny());
				v2.push_back(planar->at(nn_index).nz());
				Math_methods::angle_btw_2_vectors(v1,v2,angle);
				if ( (angle*r2d) > angular_uncertainty ) planar_indices_to_include.push_back(index);
			}
		}
	}

	return planar_indices_to_include;
}

std::vector<int> Get_Tangent_STL_Vector_Indices_With_Large_Residuals( const std::vector<Tangent> *tangent, const double &angular_uncertainty, const double &avg_nn_distance )
{
	// Function will intelligently* get the indices within the STL vector of tangent points that have large residuals
	// Intelligently* : Doesn't blindly capture all points with large residuals 
	//                 - Considers the magnitude of the residuals
	//                 - Considers the distance to other large residual points
	//                 - Considers the variability with close large residual points

	std::vector<int> tangent_indices_to_include; // what we are going to function on function exit

	double r2d = 57.29577951308232;
	double Large_distance = 1000000000;

	std::vector < double > large_tangent_residuals;
	std::vector < int > large_tangent_residuals_indices;
	for (int j = 0; j < (int)tangent->size(); j++ ){
		double grad_err = tangent->at(j).residual()*r2d;
		if ( grad_err > angular_uncertainty )
		{
			large_tangent_residuals.push_back(grad_err);
			large_tangent_residuals_indices.push_back(j);
		}
	}
	if ( large_tangent_residuals.size() != 0)
	{
		// Residual Magnitude Condition
		// sort all residuals over the threshold(angular_uncertainty) in smallest to largest - along with linked indices
		Math_methods::sort_vector_w_index(large_tangent_residuals,large_tangent_residuals_indices);
		// Will always accept largest residual over the threshold 
		tangent_indices_to_include.push_back(large_tangent_residuals_indices[(int)large_tangent_residuals_indices.size() - 1]);
		large_tangent_residuals.pop_back(); // probably don't need to do this since we never use it again
		large_tangent_residuals_indices.pop_back();

		for (int j = 0; j < (int)large_tangent_residuals_indices.size(); j++ ){
			// current index being analyzed (next largest residual in list): 
			int index = large_tangent_residuals_indices[(int)large_tangent_residuals_indices.size()- j - 1];
			// find closest point currently in tangent_indices_to_include[] to index
			double nn_dist = Large_distance;
			int nn_index = -1; // should always be overwritten. Will get a seg fault if this isn't the case
			for (int k = 0; k < (int)tangent_indices_to_include.size(); k++ ){
				double dist = distance_btw_pts(tangent->at(index),tangent->at(tangent_indices_to_include[k]));
				if ( dist < nn_dist )
				{
					nn_dist = dist;
					nn_index = tangent_indices_to_include[k];
				}
			}
			// Distance to other Large Residual Points Condition
			if ( nn_dist  > avg_nn_distance) tangent_indices_to_include.push_back(index);
			else
			{
				// Variability Condition
				// sometimes there are points closely together that exhibit a large amount of variability 
				// determine if that is the care - if so, include point
				// compare measurement of index to nn_index
				double angle = 0.0;
				std::vector<double> v1;
				std::vector<double> v2;
				v1.push_back(tangent->at(index).tx());
				v1.push_back(tangent->at(index).ty());
				v1.push_back(tangent->at(index).tz());
				v2.push_back(tangent->at(nn_index).tx());
				v2.push_back(tangent->at(nn_index).ty());
				v2.push_back(tangent->at(nn_index).tz());
				Math_methods::angle_btw_2_vectors(v1,v2,angle);
				if ( (angle*r2d) > angular_uncertainty ) tangent_indices_to_include.push_back(index);
			}
		}
	}

	return tangent_indices_to_include;
}