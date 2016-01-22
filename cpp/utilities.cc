#include "utilities.h"

namespace purify {
	namespace utilities {

	utilities::vis_params read_visibility(const std::string& vis_name)
	  {
	    /*
	      Reads an csv file with u, v, visibilities and returns the vectors.

	      vis_name:: name of input text file containing [u, v, real(V), imag(V)] (separated by ' ').
	    */
	    std::ifstream temp_file(vis_name);
	    t_int row = 0;
	    std::string line;
	    //counts size of vis file
	    while (std::getline(temp_file, line))
	      ++row;
	    Vector<t_real> utemp(row);
	    Vector<t_real> vtemp(row);
	    Vector<t_complex> vistemp(row);
	    Vector<t_complex> weightstemp(row);
	    std::ifstream vis_file(vis_name);

	    // reads in vis file
	    row = 0;
	    t_real real;
	    t_real imag;
	    std::string s;
	    std::string  entry;
	    while (vis_file)
	    {
	      if (!std::getline(vis_file, s)) break;
	      std::istringstream ss(s);
	      std::getline(ss, entry, ' ');
	      utemp(row) = std::stod(entry);
	      std::getline(ss, entry, ' ');
	      vtemp(row) = std::stod(entry);
	      std::getline(ss, entry, ' ');
	      real = std::stod(entry);
	      std::getline(ss, entry, ' ');
	      imag = std::stod(entry);
	      vistemp(row) = t_complex(real, imag);
	      std::getline(ss, entry, ' ');
	      weightstemp(row) = 1/(std::stod(entry) * std::stod(entry));
	      ++row;
	    }
	    utilities::vis_params uv_vis;
	    uv_vis.u = utemp;
	    uv_vis.v = -vtemp; // found that a reflection is needed for the orientation of the gridded image to be correct
	    uv_vis.vis = vistemp;
	    uv_vis.weights = weightstemp;
	    return uv_vis;
	  }

	  utilities::vis_params set_cell_size(const utilities::vis_params& uv_vis, t_real cell_size_u, t_real cell_size_v)
	  {
	      /*
	        Converts the units of visibilities to units of 2 * pi, while scaling for the size of a pixel (cell_size)

	        uv_vis:: visibilities
	        cell_size:: size of a pixel in arcseconds
	      */

	      utilities::vis_params scaled_vis;

	      if (cell_size_u == 0 and cell_size_v == 0)
	      {
	        Vector<t_real> u_dist = uv_vis.u.array() * uv_vis.u.array();
	        t_real max_u = std::sqrt(u_dist.maxCoeff());
	        cell_size_u = (180 * 3600) / max_u / purify_pi / 3 * 1.02; //Calculate cell size if not given one

	        Vector<t_real> v_dist = uv_vis.v.array() * uv_vis.v.array();
	        t_real max_v = std::sqrt(v_dist.maxCoeff());
	        cell_size_v = (180 * 3600) / max_v / purify_pi / 3  * 1.02; //Calculate cell size if not given one
	        std::cout << "PSF has a FWHM of " << cell_size_u * 3 << " x " << cell_size_v * 3 << " arcseconds" << '\n';
	      }
	      if (cell_size_v == 0)
	      {
	        cell_size_v = cell_size_u;
	      }

	      
	      std::cout << "Using a pixel size of " << cell_size_u << " x " << cell_size_v << " arcseconds" << '\n';

	      t_real scale_factor_u = 180 * 3600 / cell_size_u / purify_pi;
	      t_real scale_factor_v = 180 * 3600 / cell_size_v / purify_pi;
	      scaled_vis.u = uv_vis.u / scale_factor_u * 2 * purify_pi;
	      scaled_vis.v = uv_vis.v / scale_factor_v * 2 * purify_pi;
	      scaled_vis.vis = uv_vis.vis;
	      scaled_vis.weights = uv_vis.weights;
	      return scaled_vis;
	  }

	  Vector<t_complex> apply_weights(const Vector<t_complex> visiblities, const Vector<t_complex> weights)
	  {
	    /*
	      Applies weights to visiblities, assuming they are 1/sigma^2.
	    */
	    Vector<t_complex> weighted_vis;
	    weighted_vis = (visiblities.array() * weights.array()).matrix();
	    return weighted_vis;
	  }

	  utilities::vis_params uv_scale(const utilities::vis_params& uv_vis, const t_int& ftsizeu, const t_int& ftsizev)
	  {
	    /*
	      scales the uv coordinates from being in units of 2 * pi to units of pixels.
	    */
	      utilities::vis_params scaled_vis;
	      scaled_vis.u = uv_vis.u / (2 * purify_pi) * ftsizeu;
	      scaled_vis.v = uv_vis.v / (2 * purify_pi) * ftsizev;
	      scaled_vis.vis = uv_vis.vis;
	      scaled_vis.weights = uv_vis.weights;
	      return scaled_vis;
	  }

	  utilities::vis_params uv_symmetry(const utilities::vis_params& uv_vis)
	  {
	    /*
	      Adds in reflection of the fourier plane using the condjugate symmetry for a real image.

	      uv_vis:: uv coordinates for the fourier plane
	    */
	    t_int total = uv_vis.u.size();
	    Vector<t_real> utemp(2 * total);
	    Vector<t_real> vtemp(2 * total);
	    Vector<t_complex> vistemp(2 * total);
	    Vector<t_complex> weightstemp(2 * total);
	    utilities::vis_params conj_vis;
	    for (t_int i = 0; i < uv_vis.u.size(); ++i)
	    {
	      utemp(i) = uv_vis.u(i);
	      vtemp(i) = uv_vis.v(i);
	      vistemp(i) = uv_vis.vis(i);
	      weightstemp(i) = uv_vis.weights(i);
	    }
	    for (t_int i = total; i < 2 * total; ++i)
	    {
	      utemp(i) = -uv_vis.u(i - total);
	      vtemp(i) = -uv_vis.v(i - total);
	      vistemp(i) = std::conj(uv_vis.vis(i - total));
	      weightstemp(i) = uv_vis.weights(i - total);
	    }
	    conj_vis.u = utemp;
	    conj_vis.v = vtemp;
	    conj_vis.vis = vistemp;
	    conj_vis.weights = weightstemp;
	    return conj_vis;
	  }
	
	  t_int sub2ind(const t_int& row, const t_int& col, const t_int& rows, const t_int& cols) 
	  {
	    /*
	      Converts (row, column) of a matrix to a single index. This does the same as the matlab funciton sub2ind, converts subscript to index.

	      row:: row of matrix (y)
	      col:: column of matrix (x)
	      cols:: number of columns for matrix
	      rows:: number of rows for matrix
	     */
	    return row * cols + col;
	  }

	  void ind2sub(const t_int sub, const t_int cols, const t_int rows, t_int* row, t_int* col) 
	  {
	    /*
	      Converts index of a matrix to (row, column). This does the same as the matlab funciton sub2ind, converts subscript to index.
	      
	      sub:: subscript of entry in matrix
	      cols:: number of columns for matrix
	      rows:: number of rows for matrix
	      row:: output row of matrix
	      col:: output column of matrix

	     */
	    *col = sub % cols;
	    *row = (sub - *col) / cols;
	  }

	  t_int mod(const t_real& x, const t_real& y) 
	  {
	    /*
	      returns x % y, and warps circularly around y for negative values.
	    */
	      t_real r = std::fmod(x, y);
	      if (r < 0)
	        r = y + r;
	      return static_cast<t_int>(r);
	  }

	  void writefits2d(Image<t_real> eigen_image, const std::string& fits_name, const bool& overwrite, const bool& flip) 
	  {
	    /*
	      Writes an image to a fits file.

	      image:: image data, a 2d Image.
	      fits_name:: string contating the file name of the fits file.
	    */
	    if (overwrite == true) {remove(fits_name.c_str());};
	    std::auto_ptr<CCfits::FITS> pFits(0);
	    long naxes[2] = {static_cast<long>(eigen_image.rows()), static_cast<long>(eigen_image.cols())};
	    pFits.reset(new CCfits::FITS(fits_name, FLOAT_IMG, 2, naxes));
	    long fpixel (1);
	    std::vector<long> extAx; 
	    extAx.push_back(naxes[0]);
	    extAx.push_back(naxes[1]);
	    CCfits::ExtHDU* imageExt = pFits->addImage(fits_name, FLOAT_IMG, extAx);
	    if (flip == true)
	    {
	      eigen_image = eigen_image.rowwise().reverse().eval();
	    }
	    eigen_image.resize(naxes[0]*naxes[1], 1);
	    std::valarray<double> image(naxes[0]*naxes[1]);
	    for (int i = 0; i < static_cast<int>(naxes[0]*naxes[1]); ++i)
	    {
	      image[i] = static_cast<float>(eigen_image(i));
	    }
	    imageExt->write(fpixel, naxes[0]*naxes[1], image);
	  }
	  
	  Image<t_complex> readfits2d(const std::string& fits_name)
	  {
	    /*
	      Reads in an image from a fits file and returns the image.

	      fits_name:: name of fits file
	    */

	    std::auto_ptr<CCfits::FITS> pInfile(new CCfits::FITS(fits_name, CCfits::Read, true));
	    std::valarray<t_real>  contents;
	    CCfits::PHDU& image = pInfile->pHDU();
	    image.read(contents);
	    t_int ax1(image.axis(0));
	    t_int ax2(image.axis(1));
	    Image<t_complex> eigen_image(ax1, ax2);
	    t_int index;
	    for (t_int i = 0; i < ax1; ++i)
	    {
	      for (t_int j = 0; j < ax2; ++j)
	      {
	        index = sub2ind(j, i, ax2, ax1);
	        eigen_image(i, j) = contents[index];
	      }
	    }
	    return eigen_image;
	  }
	}
}