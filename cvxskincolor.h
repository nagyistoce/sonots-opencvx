/**
// cvskincolor.h
//
// Copyright (c) 2008, Naotoshi Seo. All rights reserved.
//
// The program is free to use for non-commercial academic purposes,
// but for course works, you must understand what is going inside to 
// use. The program can be used, modified, or re-distributed for any 
// purposes only if you or one of your group understand not only 
// programming codes but also theory and math behind (if any). 
// Please contact the authors if you are interested in using the 
// program without meeting the above conditions.
//
// See skincrop/skincrop.cpp as an example
*/
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )
#pragma comment( lib, "cv.lib" )
#pragma comment( lib, "cxcore.lib" )
#pragma comment( lib, "cvaux.lib" )
#endif

#include "cv.h"
#include "cvaux.h"
#include <stdio.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include "cvxgausspdf.h"
#include "cvxmorphological.h"

#ifndef CV_SKINCOLOR_INCLUDED
#define CV_SKINCOLOR_INCLUDED

/**
// cvSkinColorGMM - Skin Color Detection with GMM model
//
// @param img        Input image
// @param mask       Generated mask image. 1 for skin and 0 for others
// @param threshold  Threshold value for likelihood-ratio test
//     The preferrable threshold = (number of other pixels / number of skin pixels)
//     You may guess this value by looking your image. 
//     You may reduce this number when you can allow larger false alaram rate which
//     results in to reduce reduce miss detection rate.
// @param [probs = NULL] The likelihood-ratio valued array rather than mask if you want
// 
// References)
//  @article{606260,
//      author = {Michael J. Jones and James M. Rehg},
//      title = {Statistical color models with application to skin detection},
//      journal = {Int. J. Comput. Vision},
//      volume = {46},
//      number = {1},
//      year = {2002},
//      issn = {0920-5691},
//      pages = {81--96},
//      doi = {http://dx.doi.org/10.1023/A:1013200319198},
//      publisher = {Kluwer Academic Publishers},
//      address = {Hingham, MA, USA},
//  }
*/
void cvSkinColorGmm( const IplImage* _img, IplImage* mask, double threshold = 1.0, IplImage* probs = NULL )
{
    CV_FUNCNAME( "cvSkinColorGmm" );
    __BEGIN__;
    CV_ASSERT( _img->width == mask->width && _img->height == mask->height );
    CV_ASSERT( _img->nChannels >= 3 && mask->nChannels == 1 );
    if( probs )
    {
        CV_ASSERT( _img->width == probs->width && _img->height == probs->height );
        CV_ASSERT( probs->nChannels == 1 );
    }

    const int N = _img->height * _img->width;
    const int D = 3;
    const int K = 16;
    IplImage* img = cvCreateImage( cvGetSize(_img), _img->depth, _img->nChannels );
    cvCvtColor( _img, img, CV_BGR2RGB );

    double skin_mean[] = {
        73.5300, 249.7100, 161.6800, 186.0700, 189.2600, 247.0000, 150.1000, 206.8500, 212.7800, 234.8700, 151.1900, 120.5200, 192.2000, 214.2900,  99.5700, 238.8800,
        29.9400, 233.9400, 116.2500, 136.6200,  98.3700, 152.2000,  72.6600, 171.0900, 152.8200, 175.4300,  97.7400,  77.5500, 119.6200, 136.0800,  54.3300, 203.0800,
        17.7600, 217.4900,  96.9500, 114.4000,  51.1800,  90.8400,  37.7600, 156.3400, 120.0400, 138.9400,  74.5900,  59.8200,  82.3200,  87.2400,  38.0600, 176.9100 
    };
    double skin_cov[] = { // only diagonal components
        765.4000,  39.9400, 291.0300, 274.9500, 633.1800,  65.2300, 408.6300, 530.0800, 160.5700, 163.8000, 425.4000, 330.4500, 152.7600, 204.9000, 448.1300, 178.3800,
        121.4400, 154.4400,  60.4800,  64.6000, 222.4000, 691.5300, 200.7700, 155.0800,  84.5200, 121.5700,  73.5600,  70.3400,  92.1400, 140.1700,  90.1800, 156.2700,
        112.8000, 396.0500, 162.8500, 198.2700, 250.6900, 609.9200, 257.5700, 572.7900, 243.9000, 279.2200, 175.1100, 151.8200, 259.1500, 270.1900, 151.2900, 404.9900
    };
    double skin_weight[] = {
        0.0294, 0.0331, 0.0654, 0.0756, 0.0554, 0.0314, 0.0454, 0.0469, 0.0956, 0.0763, 0.1100, 0.0676, 0.0755, 0.0500, 0.0667, 0.0749
    };
    double nonskin_mean[] = {
        254.3700, 9.3900,  96.5700, 160.4400,  74.9800, 121.8300, 202.1800, 193.0600,  51.8800,  30.8800,  44.9700, 236.0200, 207.8600,  99.8300, 135.0600, 135.9600,
        254.4100, 8.0900,  96.9500, 162.4900,  63.2300,  60.8800, 154.8800, 201.9300,  57.1400,  26.8400,  85.9600, 236.2700, 191.2000, 148.1100, 131.9200, 103.8900,
        253.8200, 8.5200,  91.5300, 159.0600,  46.3300,  18.3100,  91.0400, 206.5500,  61.5500,  25.3200, 131.9500, 230.7000, 164.1200, 188.1700, 123.1000,  66.8800  
    };
    double nonskin_cov[] = { // only diagonal components
        2.77,  46.84, 280.69, 355.98, 414.84, 2502.2, 957.42, 562.88, 344.11, 222.07, 651.32, 225.03, 494.04, 955.88, 350.35, 806.44,
        2.81,  33.59, 156.79, 115.89, 245.95, 1383.5, 1766.9, 190.23, 191.77, 118.65, 840.52, 117.29, 237.69, 654.95,  130.3,  642.2,
        5.46,  32.48, 436.58, 591.24, 361.27, 237.18, 1582.5, 447.28,  433.4, 182.41, 963.67, 331.95, 533.52,  916.7, 388.43, 350.36
    };
    double nonskin_weight[] = {
        0.0637, 0.0516, 0.0864, 0.0636, 0.0747, 0.0365, 0.0349, 0.0649, 0.0656, 0.1189, 0.0362, 0.0849, 0.0368, 0.0389, 0.0943, 0.0477
    };

    // transform to CvMat
    CvMat* SkinMeans = &cvMat( D, K, CV_64FC1, skin_mean );
    CvMat* SkinWeights = &cvMat( 1, K, CV_64FC1, skin_weight );
    CvMat **SkinCovs = (CvMat**)cvAlloc( K * sizeof(*SkinCovs) );

    for( int k = 0; k < K; k++ )
    {
        double cov_tmp[D][D]; // diagonal
        for( int i = 0; i < D; i++ )
            for( int j = 0; j < D; j++ )
                cov_tmp[i][j] = 0;
        for( int i = 0; i < D; i++ )
            cov_tmp[i][i] = skin_cov[K * i + k];
        SkinCovs[k] = &cvMat( D, D, CV_64FC1, cov_tmp );
    }

    // transform to CvMat
    CvMat* NonSkinMeans = &cvMat( D, K, CV_64FC1, nonskin_mean );
    CvMat* NonSkinWeights = &cvMat( 1, K, CV_64FC1, nonskin_weight );
    CvMat **NonSkinCovs = (CvMat**)cvAlloc( K * sizeof(*NonSkinCovs) );

    for( int k = 0; k < K; k++ )
    {
        double cov_tmp[D][D]; // diagonal
        for( int i = 0; i < D; i++ )
            for( int j = 0; j < D; j++ )
                cov_tmp[i][j] = 0;
        for( int i = 0; i < D; i++ )
            cov_tmp[i][i] = nonskin_cov[K * i + k];
        NonSkinCovs[k] = &cvMat( D, D, CV_64FC1, cov_tmp );
    }

    // reshape IplImage to D (colors) x N matrix of CV_64FC1
    CvMat *Mat = cvCreateMat( D, N, CV_64FC1 );
    CvMat *PreMat, hdr;
    PreMat = cvCreateMat( img->height, img->width, CV_64FC3 );
    cvConvert( img, PreMat ); 
    cvTranspose( cvReshape( PreMat, &hdr, 1, N ), Mat );
    cvReleaseMat( &PreMat );

    // GMM PDF
    CvMat *SkinProbs = cvCreateMat(1, N, CV_64FC1);
    CvMat *NonSkinProbs = cvCreateMat(1, N, CV_64FC1);
    cvMatGmmPdf( Mat, SkinMeans, SkinCovs, SkinWeights, SkinProbs, true);
    cvMatGmmPdf( Mat, NonSkinMeans, NonSkinCovs, NonSkinWeights, NonSkinProbs, true);

    // Likelihood-ratio test
    CvMat *Mask = cvCreateMat( 1, N, CV_8UC1 );
    cvDiv( SkinProbs, NonSkinProbs, SkinProbs );
    cvThreshold( SkinProbs, Mask, threshold, 1, CV_THRESH_BINARY );
    cvConvert( cvReshape( Mask, &hdr, 1, img->height ), mask );
    
    if( probs ) cvConvert( cvReshape( SkinProbs, &hdr, 1, img->height ), probs );

    cvFree( &SkinCovs );
    cvFree( &NonSkinCovs );

    cvReleaseMat( &SkinProbs );
    cvReleaseMat( &NonSkinProbs );
    cvReleaseMat( &Mask );
    cvReleaseImage( &img );

    __END__;
}

/**
// cvSkinColorGauss - Skin Color Detection with a Gaussian model
//
// @param img    Input image
// @param mask   Generated mask image. 1 for skin and 0 for others
// @param [factor = 2.5] A factor to determine threshold value.
//     The default threshold is -2.5 * sigma and 2.5 * sigma which
//     supports more than 95% region of Gaussian PDF. 
// 
// References)
//  [1] @INPROCEEDINGS{Yang98skin-colormodeling,
//     author = {Jie Yang and Weier Lu and Alex Waibel},
//     title = {Skin-color modeling and adaptation},
//     booktitle = {},
//     year = {1998},
//     pages = {687--694}
//  }
//  [2] C. Stauffer, and W. E. L. Grimson, “Adaptive
//  background mixture models for real-time tracking”, In:
//  Proceedings 1999 IEEE Computer Society Conference
//  on Computer Vision and Pattern Recognition (Cat. No
//  PR00149), IEEE Comput. Soc. Part vol. 2, 1999.
*/
void cvSkinColorGauss( const IplImage* _img, IplImage* mask, double factor = 2.5 )
{
    double mean[] = { 188.9069, 142.9157, 115.1863 };
    double sigma[] = { 58.3542, 45.3306, 43.397 };
    double subdata[3];

    IplImage* img = cvCreateImage( cvGetSize(_img), _img->depth, _img->nChannels );
    cvCvtColor( _img, img, CV_BGR2RGB );
    CvMat* mat = cvCreateMat( img->height, img->width, CV_64FC3 );
    cvConvert( img, mat );

    for( int i = 0; i < mat->rows; i++ )
    {
        for( int j = 0; j < mat->cols; j++ )
        {
            bool skin;
            CvScalar data = cvGet2D( mat, i, j );
            subdata[0] = data.val[0] - mean[0];
            subdata[1] = data.val[1] - mean[1];
            subdata[2] = data.val[2] - mean[2];
            //if( CV_SKINCOLOR_GAUSS_CUBE ) // cube-like judgement
            //{
                skin = 1;
                // 2 * sigma => 95% confidence region, 2.5 gives more
                skin &= ( - factor *sigma[0] < subdata[0] && subdata[0] <  factor *sigma[0] );
                skin &= ( - factor *sigma[1] < subdata[1] && subdata[1] <  factor *sigma[1] );
                skin &= ( - factor *sigma[2] < subdata[2] && subdata[2] <  factor *sigma[2] );
            //}
            //else if( CV_SKINCOLOR_GAUSS_ELLIPSOID ) // ellipsoid-like judgement
            //{
            //    double val = 0;
            //    val += pow( subdata[0] / sigma[0] , 2);
            //    val += pow( subdata[1] / sigma[1] , 2);
            //    val += pow( subdata[2] / sigma[2] , 2);
            //    skin = ( val <= pow(  factor , 2 ) );
            //}
            //else if( CV_SKINCOLOR_GAUSS_LIKELIHOODRATIO ) // likelihood-ratio test (need data for non-skin gaussain model)
            //{
            //}
            mask->imageData[mask->widthStep * i + j] = skin;
        }
    }
    cvReleaseMat( &mat );
    cvReleaseImage( &img );
}

/**
// cvSkinColorSimple - Skin Color Detection by Peer, et.al [1]
//
// @param img Input image
// @param mask Generated mask image. 1 for skin and 0 for others
// 
// References)
//  [1] P. Peer, J. Kovac, J. and F. Solina, ”Human skin colour
//  clustering for face detection”, In: submitted to EUROCON –
//  International Conference on Computer as a Tool , 2003.
//     (R>95)^(G>40)^(B>20)^
//     (max{R,G,B}-min{R,G,B}>15)^
//     (|R -G|>15)^(R>G)^(R>B)
*/
void cvSkinColorSimple( const IplImage* img, IplImage* mask )
{
    cvSet( mask, cvScalarAll(0) );
    for( int y = 0; y < img->height; y++ )
    {
        for( int x = 0; x < img->width; x++ )
        {
            uchar b = img->imageData[img->widthStep * y + x * 3];
            uchar g = img->imageData[img->widthStep * y + x * 3 + 1];
            uchar r = img->imageData[img->widthStep * y + x * 3 + 2];

            if( r > 95 && g > 40 && b > 20 && 
                max( max( r, g ), b ) - min( min( r, g ), b ) > 15 &&
                abs( r - g ) > 15 && r > g && r > b )
            {
                mask->imageData[mask->widthStep * y + x] = 1;
            }
        }
    }
}

/**
// cvSkinColorCbCr - Skin Color Detection in (Cb, Cr) space by [1][2]
//
// @param img  Input image
// @param mask Generated mask image. 1 for skin and 0 for others
// @param [dist = NULL] The distortion valued array rather than mask if you want
// 
// References)
//  [1] R.L. Hsu, M. Abdel-Mottaleb, A.K. Jain, "Face Detection in Color Images," 
//  IEEE Transactions on Pattern Analysis and Machine Intelligence ,vol. 24, no. 5,  
//  pp. 696-706, May, 2002. (Original)
//  [2] P. Peer, J. Kovac, J. and F. Solina, ”Human skin colour
//  clustering for face detection”, In: submitted to EUROCON –
//  International Conference on Computer as a Tool , 2003. (Tuned)
*/
void cvSkinColorCrCb( const IplImage* _img, IplImage* mask, CvArr* distarr = NULL )
{
    CV_FUNCNAME( "cvSkinColorCbCr" );
    __BEGIN__;
    int width  = _img->width;
    int height = _img->height;
    CvMat* dist = (CvMat*)distarr, diststub;
    int coi = 0;

    CV_ASSERT( width == mask->width && height == mask->height );
    CV_ASSERT( _img->nChannels >= 3 && mask->nChannels == 1 );

    if( dist && !CV_IS_MAT(dist) )
    {
        CV_CALL( dist = cvGetMat( dist, &diststub, &coi ) );
        if (coi != 0) CV_ERROR_FROM_CODE(CV_BadCOI);
        CV_ASSERT( width == dist->cols && height == dist->rows );
        CV_ASSERT( CV_MAT_TYPE(dist->type) == CV_32FC1 || CV_MAT_TYPE(dist->type) == CV_64FC1 );
    }

    IplImage* img = cvCreateImage( cvGetSize(_img), IPL_DEPTH_8U, 3 );
    cvCvtColor( _img, img, CV_BGR2YCrCb );        

    double Wcb = 46.97;
    double WLcb = 23;
    double WHcb = 14;
    double Wcr = 38.76;
    double WLcr = 20;
    double WHcr = 10;
    double Kl = 125;
    double Kh = 188;
    double Ymin = 16;
    double Ymax = 235;
    double alpha = 0.56;

    double Cx = 109.38;
    double Cy = 152.02;
    double theta = 2.53; 
    double ecx = 1.6;
    double ecy = 2.41;
    double a = 25.39;
    double b = 14.03;

    cvSet( mask, cvScalarAll(0) );
    for( int row = 0; row < height; row++ )
    {
        for( int col = 0; col < width; col++ )
        {
            uchar Y  = img->imageData[img->widthStep * row + col * 3];
            uchar Cr = img->imageData[img->widthStep * row + col * 3 + 1];
            uchar Cb = img->imageData[img->widthStep * row + col * 3 + 2];

            double Cb_Y, Cr_Y, Wcb_Y, Wcr_Y;
            if( Y < Kl )
                Wcb_Y = WLcb + (Y - Ymin) * (Wcb - WLcb) / (Kl - Ymin);
            else if( Kh < Y )
                Wcb_Y = WHcb + (Ymax - Y) * (Wcb - WHcb) / (Ymax - Kh);
            else
                Wcb_Y = Wcb;

            if( Y < Kl )
                Wcr_Y = WLcr + (Y - Ymin) * (Wcr - WLcr) / (Kl - Ymin);
            else if( Kh < Y )
                Wcr_Y = WHcr + (Ymax - Y) * (Wcr - WHcr) / (Ymax - Kh);
            else
                Wcr_Y = Wcr;

            if( Y < Kl )
                Cb_Y = 108 + (Kl - Y) * 10 / ( Kl - Ymin );
            else if( Kh < Y )
                Cb_Y = 108 + (Y - Kh) * 10 / ( Ymax - Kh );
            else
                Cb_Y = 108;

            if( Y < Kl )
                Cr_Y = 154 - (Kl - Y) * 10 / ( Kl - Ymin );
            else if( Kh < Y )
                Cr_Y = 154 + (Y - Kh) * 22 / ( Ymax - Kh );
            else
                Cr_Y = 108;

            double x = cos(theta) * (Cb - Cx) + sin(theta) * (Cr - Cy);
            double y = -1 * sin(theta) * (Cb - Cx) + cos(theta) * (Cr - Cy);

            double distort = pow(x-ecx,2) / pow(a,2) + pow(y-ecy,2) / pow(b,2);
            if( dist )
                cvmSet( dist, row, col, distort );

            if( distort <= 1 )
            {
                mask->imageData[mask->widthStep * row + col] = 1;
            }
        }
    }
    __END__;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
