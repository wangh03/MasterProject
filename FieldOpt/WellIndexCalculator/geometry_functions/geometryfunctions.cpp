#include "geometryfunctions.h"
#include "geometryfunctions_exceptions.h"

namespace WellIndexCalculator {

    namespace GeometryFunctions {

        Eigen::Vector3d line_plane_intersection(Eigen::Vector3d p0, Eigen::Vector3d p1,
                                                                   Eigen::Vector3d normal_vector,
                                                                   Eigen::Vector3d point_in_plane) {

            Eigen::Vector3d line_vector = Eigen::Vector3d(p1.x() - p0.x(), p1.y() - p0.y(), p1.z() - p0.z());

            /* Some numerical issues when the line_vector (vector between p0 and p1)
             * is much longer (or shorter) than the normal vector. Normalize both
             * to fix the issue. Resultins parameter s further down should now
             * be more numerically stable.
             */
            line_vector.normalize();
            normal_vector.normalize();

            Eigen::Vector3d w = Eigen::Vector3d(p0.x() - point_in_plane.x(), p0.y() - point_in_plane.y(),
                                                p0.z() - point_in_plane.z());

            /* s is a variable for the parametrized line defined by the 2 points in line.
             *Inputting a s such that s>=0 or s<=1 will give a point on the line between the 2 points in line.
             */
            double s = normal_vector.dot(-w) / normal_vector.dot(line_vector);

            // Use the found s in parametrizaton to return intersection point.
            Eigen::Vector3d intersection_point = Eigen::Vector3d(p0.x() + s * (line_vector.x()),
                                                                 p0.y() + s * (line_vector.y()),
                                                                 p0.z() + s * (line_vector.z()));

            return intersection_point;
        }

        Eigen::Vector3d normal_vector(Eigen::Vector3d p0, Eigen::Vector3d p1, Eigen::Vector3d p2) {
            Eigen::Vector3d normal_vector = (p2 - p0).cross(p1 - p0);
            return normal_vector.normalized();
        }

        bool point_on_same_side(Eigen::Vector3d point, Eigen::Vector3d plane_point,
                                                   Eigen::Vector3d normal_vector, double slack) {
            /* The dot product helps us determine if the angle between the two
             * vectors is below (positive answer), at (zero answer) or above
             * (negative answer) 90 degrees. Essentially telling us which side
             * of a plane the point is
             */
            double dot_product = (point - plane_point).dot(normal_vector);
            return dot_product >= 0.0 - slack;
        }

        QPair<QList<int>, QList<Eigen::Vector3d>> cells_intersected(Eigen::Vector3d start_point,
                                                                                       Eigen::Vector3d end_point,
                                                                                       Reservoir::Grid::Grid *grid) {
            // Lists which will contain intersected block indeces and intersection points.
            QList<int> cell_global_index;
            QList<Eigen::Vector3d> entry_points;

            /* Find first and last cell blocks intersected and their indeces.
             * Add first cell and first point to lists.
             */
            Reservoir::Grid::Cell last_cell = get_cell_enveloping_point(grid, end_point);
            Reservoir::Grid::Cell first_cell = get_cell_enveloping_point(grid, start_point);

            int last_cell_index = last_cell.global_index();
            int first_cell_index = first_cell.global_index();
            cell_global_index.append(first_cell_index);
            entry_points.append(start_point);

            /* If first and last block are the same then this block is
             * the only one intercepted. Return current cell and start point + end point.
             */
            if (last_cell_index == first_cell_index) {
                entry_points.append(end_point);
                QPair<QList<int>, QList<Eigen::Vector3d>> early_pair;
                early_pair.first = cell_global_index;
                early_pair.second = entry_points;
                return early_pair;
            }


            Eigen::Vector3d exit_point = find_exit_point(first_cell, start_point, end_point, start_point);
            // Make sure we follow line in the correct direction. (i.e. dot product positive)
            if ((end_point - start_point).dot(exit_point - start_point) <= 0.0) {
                std::cout << "wrong direction, try other" << std::endl;
                std::cout << "exit_point = " << exit_point.x() << " " << exit_point.y() << " " << exit_point.z() <<
                std::endl;
                exit_point = find_exit_point(first_cell, start_point, end_point, exit_point);
            }
            double epsilon = 0.01 / (end_point - exit_point).norm();
            Eigen::Vector3d move_exit_epsilon = exit_point * (1 - epsilon) + end_point * epsilon;

            Reservoir::Grid::Cell current_cell = get_cell_enveloping_point(grid, move_exit_epsilon);
            double epsilon_temp = epsilon;
            while (current_cell.global_index() == first_cell_index) {
                epsilon_temp = 10 * epsilon_temp;
                move_exit_epsilon = exit_point * (1 - epsilon_temp) + end_point * epsilon_temp;
                current_cell = get_cell_enveloping_point(grid, move_exit_epsilon);
            }

            while (current_cell.global_index() != last_cell_index) {

                // Add current cell and previous exit point to lists
                cell_global_index.append(current_cell.global_index());
                entry_points.append(exit_point);

                // Find other exit point.
                exit_point = find_exit_point(current_cell, exit_point, end_point, exit_point);

                // DO SOME CHECK IF EXIT POINT IS THE SAME AS END_POINT: UNLIKELY IN PRACTICE
                if (exit_point == end_point) {
                    current_cell = last_cell;
                }
                    // Move slightly along line to enter the new cell and find cell by using GetCellEnvelopingPoint function.
                else {
                    epsilon = 0.01 / (end_point - exit_point).norm();
                    move_exit_epsilon = exit_point * (1 - epsilon) + end_point * epsilon;
                    current_cell = get_cell_enveloping_point(grid, move_exit_epsilon);
                }

            }
            cell_global_index.append(last_cell_index);
            entry_points.append(exit_point);
            entry_points.append(end_point);

            /* Collect global indeces of intersected cells and the
             * endpoints of the line segment inside each cell in a
             * QPair type to return them both
             */
            QPair<QList<int>, QList<Eigen::Vector3d>> pair;
            pair.first = cell_global_index;
            pair.second = entry_points;
            return pair;

        }

        QList<QList<Eigen::Vector3d>> cell_planes_coords(
                QList<Eigen::Vector3d> corners) {
            QList<QList<int>> points;
            QList<int> p0, p1, p2, p3, p4, p5;

            p0 << 0 << 2 << 1; //First face
            p1 << 4 << 5 << 6; //Second face
            p2 << 0 << 4 << 2; //third face
            p3 << 1 << 3 << 5; //fourth face
            p4 << 0 << 1 << 4; //fifth face
            p5 << 2 << 6 << 3; //Sixth face
            points << p0 << p1 << p2 << p3 << p4 << p5;

            /* Fill 2D QList array with 3 corner coordinates for each of the 6 faces
             * Corners have been chosen in such a way that normal_vector() function
             * returns a vector that points in towards the centre of the block.
             */
            QList<QList<Eigen::Vector3d>> face_corner_coords;
            for (int ii = 0; ii < 6; ii++) {
                QList<Eigen::Vector3d> currentSideCorners;
                currentSideCorners.append(corners[points[ii][0]]);
                currentSideCorners.append(corners[points[ii][1]]);
                currentSideCorners.append(corners[points[ii][2]]);
                face_corner_coords.append(currentSideCorners);
            }

            return face_corner_coords;
        }

        Eigen::Vector3d find_exit_point(Reservoir::Grid::Cell cell, Eigen::Vector3d entry_point,
                                                           Eigen::Vector3d end_point, Eigen::Vector3d exception_point) {
            /* takes an entry point as input and an end_point
             * which just defines the line of the well. Find
             * the two points which intersect the block faces
             * and choose the one of them which is not the entry
             * point. This will be the exit point.
             */

            Eigen::Vector3d line = end_point - entry_point;

            // First find normal vectors of all faces of block/cell.
            QList<QList<Eigen::Vector3d>> face_corner_coords = cell_planes_coords(cell.corners());
            QList<Eigen::Vector3d> normal_vectors;
            for (int ii = 0; ii < 6; ii++) {
                Eigen::Vector3d cur_normal_vector = normal_vector(face_corner_coords.at(ii).at(0),
                                                                                     face_corner_coords.at(ii).at(1),
                                                                                     face_corner_coords.at(ii).at(2));
                normal_vectors.append(cur_normal_vector);
            }

            /* For loop through all faces untill we find a face that
             * intersects with line on face of the block and not just
             * the extension of the face to a plane
             */
            for (int face_number = 0; face_number < 6; face_number++) {
                // Normal vector
                Eigen::Vector3d cur_normal_vector = normal_vectors[face_number];
                Eigen::Vector3d cur_face_point = face_corner_coords.at(face_number).at(0);
                /* If the dot product of the line vector and the face normal vector is
                 * zero then the line is paralell to the face and won't intersect it
                 * unless it lies in the same plane, which in any case won't be the
                 * exit point.
                 */

                if (cur_normal_vector.dot(line) != 0) {
                    // Finds the intersection point of line and the current face
                    Eigen::Vector3d intersect_point = line_plane_intersection(entry_point,
                                                                              end_point,
                                                                              cur_normal_vector,
                                                                              cur_face_point);

                    /* Loop through all faces and check that intersection point is on the correct side of all of them.
                     * i.e. the same direction as the normal vector of each face
                     */
                    bool feasible_point = true;
                    for (int ii = 0; ii < 6; ii++) {
                        if (!point_on_same_side(intersect_point,
                                                                   face_corner_coords.at(ii).at(0),
                                                                   normal_vectors[ii],
                                                                   10e-6)) {
                            feasible_point = false;
                        }
                    }

                    // If point is feasible(i.e. on/inside cell), not identical to given entry point, and going in correct direction
                    if (feasible_point && (exception_point - intersect_point).norm() > 10e-10
                        && (end_point - entry_point).dot(end_point - intersect_point) >= 0) {
                        return intersect_point;
                    }

                }

            }
            // If all fails, the line intersects the cell in a single point(corner or edge) -> return entry_point
            return entry_point;
        }

        Eigen::Vector3d project_v1_on_v2(Eigen::Vector3d v1, Eigen::Vector3d v2) {
            return v2 * v2.dot(v1) / v2.dot(v2);
        }

        double well_index_cell_qvector(Reservoir::Grid::Cell cell,
                                                          QList<Eigen::Vector3d> start_points,
                                                          QList<Eigen::Vector3d> end_points, double wellbore_radius) {
            /* corner points of Cell(s) are always listen in the same order and orientation. (see
             * Reservoir::Grid::Cell for illustration as it is included in the code.
             * Assumption: The block is fairly regular, i.e. corners are right angles.
             * Determine the 3(orthogonal, or very close to orth.) vectors to project line onto.
             * Corners 4&5, 4&6 and 4&0 span the cell from the front bottom left corner.
             */
            QList<Eigen::Vector3d> corners = cell.corners();
            Eigen::Vector3d xvec = corners[5] - corners[4];
            Eigen::Vector3d yvec = corners[6] - corners[4];
            Eigen::Vector3d zvec = corners[0] - corners[4];

            // Finds the dimensional sizes (i.e. length in each direction) of the cell block
            double dx = xvec.norm();
            double dy = yvec.norm();
            double dz = zvec.norm();
            // Get directional permeabilities
            double kx = cell.permx();
            double ky = cell.permy();
            double kz = cell.permz();

            double Lx = 0;
            double Ly = 0;
            double Lz = 0;

            // Need to add projections of all segments, each line is one segment.
            for (int ii = 0; ii < start_points.length(); ++ii) { // Current segment ii
                // Compute vector from segment
                Eigen::Vector3d current_vec = end_points.at(ii) - start_points.at(ii);

                /* Proejcts segment vector to directional spanning vectors and determines the length.
                 * of the projections. Note that we only only care about the length of the projection,
                 * not the spatial position. Also adds the lengths of previous segments in case there
                 * is more than one segment within the well.
                 */
                Lx = Lx + project_v1_on_v2(current_vec, xvec).norm();
                Ly = Ly + project_v1_on_v2(current_vec, yvec).norm();
                Lz = Lz + project_v1_on_v2(current_vec, zvec).norm();
            }

            // Compute Well Index from formula provided by Shu
            double well_index_x = (dir_well_index(Lx, dy, dz, ky, kz, wellbore_radius));
            double well_index_y = (dir_well_index(Ly, dx, dz, kx, kz, wellbore_radius));
            double well_index_z = (dir_well_index(Lz, dx, dy, kx, ky, wellbore_radius));
            return sqrt(well_index_x * well_index_x + well_index_y * well_index_y + well_index_z * well_index_z);
        }

        double dir_well_index(double Lx, double dy, double dz, double ky, double kz,
                                                 double wellbore_radius) {
            // wellbore radius should probably be taken as input. CAREFUL
            //double wellbore_radius = 0.1905;
            double silly_eclipse_factor = 0.008527;
            double well_index_i = silly_eclipse_factor * (2 * M_PI * sqrt(ky * kz) * Lx) /
                                  (log(dir_wellblock_radius(dy, dz, ky, kz) / wellbore_radius));
            return well_index_i;
        }

        double dir_wellblock_radius(double dx, double dy, double kx, double ky) {
            double r = 0.28 * sqrt((dx * dx) * sqrt(ky / kx) + (dy * dy) * sqrt(kx / ky)) /
                       (sqrt(sqrt(kx / ky)) + sqrt(sqrt(ky / kx)));
            return r;
        }

        bool is_point_inside_cell(Reservoir::Grid::Cell cell, Eigen::Vector3d point) {
            // First find normal vectors of all faces of block/cell.
            QList<QList<Eigen::Vector3d>> face_corner_coords = cell_planes_coords(cell.corners());
            QList<Eigen::Vector3d> normal_vectors;
            for (int ii = 0; ii < 6; ii++) {
                normal_vectors.append(normal_vector(face_corner_coords.at(ii).at(0),
                                                                       face_corner_coords.at(ii).at(1),
                                                                       face_corner_coords.at(ii).at(2)));
            }

            /* For loop through all faces to check that point
             * is on right side of every face (i.e. in the same
             * direction as the computed normal vector) by
             * taking the dot product of the normal vector with
             * the vector going from one corner of the face to
             * point
             */
            bool point_inside = true;
            for (int face_number = 0; face_number < 6; face_number++) {
                if ((point - face_corner_coords[face_number][0]).dot(normal_vectors[face_number]) < 0) {
                    point_inside = false;
                }
            }

            return point_inside;
        }

        Reservoir::Grid::Cell get_cell_enveloping_point(Reservoir::Grid::Grid *grid,
                                                                           Eigen::Vector3d point) {
            // Get total number of cells
            int total_cells = grid->Dimensions().nx * grid->Dimensions().ny * grid->Dimensions().nz;

            for (int ii = 0; ii < total_cells; ii++) {
                if (is_point_inside_cell(grid->GetCell(ii), point)) {
                    return grid->GetCell(ii);
                }
            }
            // Throw an exception if no cell was found
            throw std::runtime_error("WellIndexCalculator::get_cell_enveloping_point: Cell is outside grid ("
                                     + std::to_string(point.x()) + ", "
                                     + std::to_string(point.y()) + ", "
                                     + std::to_string(point.z()) + ")"
            );
        }

        QPair<QList<int>, QList<double> > well_index_of_grid(Reservoir::Grid::Grid *grid,
                                                                                QList<Eigen::Vector3d> start_points,
                                                                                QList<Eigen::Vector3d> end_points,
                                                                                double wellbore_radius) {
            // Find intersected blocks and the points of intersection
            QPair<QList<int>, QList<Eigen::Vector3d>> temp_pair = cells_intersected(
                    start_points.at(0), end_points.at(0), grid);
            QPair<QList<int>, QList<double>> pair;

            QList<double> well_indeces;
            for (int ii = 0; ii < temp_pair.first.length(); ii++) {
                QList<Eigen::Vector3d> entry_points;
                QList<Eigen::Vector3d> exit_points;
                entry_points.append(temp_pair.second.at(ii));
                exit_points.append(temp_pair.second.at(ii + 1));
                well_indeces.append(
                        well_index_cell_qvector(grid->GetCell(temp_pair.first.at(ii)), entry_points,
                                                                   exit_points, wellbore_radius));
            }
            pair.first = temp_pair.first;
            pair.second = well_indeces;
            return pair;
        }


//    void print_well_index_file(Reservoir::Grid::Grid *grid, QList<QVector3D> start_points, QList<QVector3D> end_points, double wellbore_radius, double min_wi, QString filename)
//    {
//        // Find intersected blocks and the points of intersection
//        QPair<QList<int>, QList<QVector3D>> temp_pair = cells_intersected(start_points.at(0),end_points.at(0),grid);
//        QPair<QList<int>, QList<double>> pair;
//
//        QList<double> well_indeces;
//        for(int ii=0; ii<temp_pair.first.length(); ii++){
//            QList<QVector3D> entry_points;
//            QList<QVector3D> exit_points;
//            entry_points.append(temp_pair.second.at(ii));
//            exit_points.append(temp_pair.second.at(ii+1));
//            well_indeces.append(well_index_cell_qvector(grid->GetCell(temp_pair.first.at(ii)),entry_points, exit_points, wellbore_radius));
//        }
//        pair.first = temp_pair.first;
//        pair.second = well_indeces;
//
//        std::ofstream myfile;
//        myfile.open (filename.toUtf8().constData());
//        myfile <<"-- ==================================================================================================\n";
//        myfile <<"-- \n";
//        myfile <<"-- Exported from ECL_5SPOT\n";
//        myfile <<"-- \n";
//        myfile <<"-- Exported by user hilmarm from WellIndexCalculator \n";
//        myfile <<"-- \n";
//        myfile <<"-- ==================================================================================================\n";
//        myfile <<"\n";
//        myfile <<"WELSPECS\n";
//        myfile <<" 'TW01' 'PRODUC'    1    1  1712.00 'OIL'    0.0 'STD' 'SHUT' 'YES' 0 'SEG' /\n";
//        myfile <<"/\n";
//        myfile <<" \n";
//        myfile <<"\n";
//        myfile <<"GRUPTREE\n";
//        myfile <<" 'PRODUC' 'FIELD' /\n";
//        myfile <<" 'INJECT' 'FIELD' /\n";
//        myfile <<"/\n";
//        myfile <<"\n";
//        myfile <<"COMPDAT\n";
//        myfile <<"-- --------------------------------------------------------------------------------------------------\n";
//        for( int ii = 0; ii<pair.first.length(); ii++){
//
//            if(pair.second.at(ii)>min_wi){
//                myfile << " 'TW01'" ;
//                myfile << std::setw(5) << grid->GetCell(pair.first.at(ii)).ijk_index().i()+1;
//                myfile << std::setw(5) << grid->GetCell(pair.first.at(ii)).ijk_index().j()+1;
//                myfile << std::setw(5) << grid->GetCell(pair.first.at(ii)).ijk_index().k()+1;
//                myfile << std::setw(5) << grid->GetCell(pair.first.at(ii)).ijk_index().k()+1;
//
//                myfile << "  'OPEN' 0 " ;
//                float temp = pair.second.at(ii);
//                myfile << std::setprecision(5) << std::fixed;
//                myfile <<std::setw(13) << temp;
//
//                myfile.unsetf(std::ios_base::fixed);
//                myfile << std::setw(8)  << wellbore_radius*2 ;
//
//                float temp2 = grid->GetCell(pair.first.at(ii)).permx()*temp_pair.second.at(ii).distanceToPoint(temp_pair.second.at(ii+1));
//                myfile << std::setprecision(5) << std::fixed;
//                myfile << std::setw(13) << temp2;
//                myfile << "    0.00 0.0 'X'    4.75 /\n";
//                myfile.unsetf(std::ios_base::fixed);
//            }
//        }
//        myfile << "-- --------------------------------------------------------------------------------------------------\n";
//        myfile << "/";
//        myfile << "\n";
//        myfile << "\n";
//
//        myfile.close();
//    }


    }
}