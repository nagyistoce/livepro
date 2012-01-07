#include "EyeCounter.h"

#include <QtGui>
#include <QDebug>

#include "opencv/cv.h"
#include "opencv/highgui.h"

void detectEyes(IplImage *img);
IplImage* QImage2IplImage(QImage qimg);
QImage IplImage2QImage(IplImage *iplImg);

#define OPENCV_DATA_DIR "/opt/OpenCV-2.1.0/data/haarcascades"

#include "/opt/OpenCV-2.1.0/src/cv/_cv.h"

const int icv_object_win_border = 1;
const float icv_stage_threshold_bias = 0.0001f;

typedef int sumtype;
typedef double sqsumtype;

typedef struct CvHidHaarFeature
{
    struct
    {
        sumtype *p0, *p1, *p2, *p3;
        float weight;
    }
    rect[CV_HAAR_FEATURE_MAX];
}
CvHidHaarFeature;


typedef struct CvHidHaarTreeNode
{
    CvHidHaarFeature feature;
    float threshold;
    int left;
    int right;
}
CvHidHaarTreeNode;


typedef struct CvHidHaarClassifier
{
    int count;
    //CvHaarFeature* orig_feature;
    CvHidHaarTreeNode* node;
    float* alpha;
}
CvHidHaarClassifier;


typedef struct CvHidHaarStageClassifier
{
    int  count;
    float threshold;
    CvHidHaarClassifier* classifier;
    int two_rects;

    struct CvHidHaarStageClassifier* next;
    struct CvHidHaarStageClassifier* child;
    struct CvHidHaarStageClassifier* parent;
}
CvHidHaarStageClassifier;


struct CvHidHaarClassifierCascade
{
    int  count;
    int  is_stump_based;
    int  has_tilted_features;
    int  is_tree;
    double inv_window_area;
    CvMat sum, sqsum, tilted;
    CvHidHaarStageClassifier* stage_classifier;
    sqsumtype *pq0, *pq1, *pq2, *pq3;
    sumtype *p0, *p1, *p2, *p3;

    void** ipp_stages;
};


namespace cv
{

struct HaarDetectObjects_ScaleImage_Invoker
{
    HaarDetectObjects_ScaleImage_Invoker( const CvHaarClassifierCascade* _cascade,
                                          int _stripSize, double _factor,
                                          const Mat& _sum1, const Mat& _sqsum1, Mat* _norm1,
                                          Mat* _mask1, Rect _equRect, ConcurrentRectVector& _vec )
    {
        cascade = _cascade;
        stripSize = _stripSize;
        factor = _factor;
        sum1 = _sum1;
        sqsum1 = _sqsum1;
        norm1 = _norm1;
        mask1 = _mask1;
        equRect = _equRect;
        vec = &_vec;
    }
    
    void operator()( const BlockedRange& range ) const
    {
        Size winSize0 = cascade->orig_window_size;
        Size winSize(cvRound(winSize0.width*factor), cvRound(winSize0.height*factor));
        int y1 = range.begin()*stripSize, y2 = min(range.end()*stripSize, sum1.rows - 1 - winSize0.height);
        Size ssz(sum1.cols - 1 - winSize0.width, y2 - y1);
        int x, y, ystep = factor > 2 ? 1 : 2;
        
    #ifdef HAVE_IPP
        if( cascade->hid_cascade->ipp_stages )
        {
            IppiRect iequRect = {equRect.x, equRect.y, equRect.width, equRect.height};
            ippiRectStdDev_32f_C1R(sum1.ptr<float>(y1), sum1.step,
                                   sqsum1.ptr<double>(y1), sqsum1.step,
                                   norm1->ptr<float>(y1), norm1->step,
                                   ippiSize(ssz.width, ssz.height), iequRect );
            
            int positive = (ssz.width/ystep)*((ssz.height + ystep-1)/ystep);

            if( ystep == 1 )
                (*mask1) = Scalar::all(1);
            else
                for( y = y1; y < y2; y++ )
                {
                    uchar* mask1row = mask1->ptr(y);
                    memset( mask1row, 0, ssz.width );
                    
                    if( y % ystep == 0 )
                        for( x = 0; x < ssz.width; x += ystep )
                            mask1row[x] = (uchar)1;
                }
            
            for( int j = 0; j < cascade->count; j++ )
            {
                if( ippiApplyHaarClassifier_32f_C1R(
                            sum1.ptr<float>(y1), sum1.step,
                            norm1->ptr<float>(y1), norm1->step,
                            mask1->ptr<uchar>(y1), mask1->step,
                            ippiSize(ssz.width, ssz.height), &positive,
                            cascade->hid_cascade->stage_classifier[j].threshold,
                            (IppiHaarClassifier_32f*)cascade->hid_cascade->ipp_stages[j]) < 0 )
                    positive = 0;
                if( positive <= 0 )
                    break;
            }
            
            if( positive > 0 )
                for( y = y1; y < y2; y += ystep )
                {
                    uchar* mask1row = mask1->ptr(y);
                    for( x = 0; x < ssz.width; x += ystep )
                        if( mask1row[x] != 0 )
                        {
                            vec->push_back(Rect(cvRound(x*factor), cvRound(y*factor),
                                                winSize.width, winSize.height));
                            if( --positive == 0 )
                                break;
                        }
                    if( positive == 0 )
                        break;
                }
        }
        else
#endif
            for( y = y1; y < y2; y += ystep )
                for( x = 0; x < ssz.width; x += ystep )
                {
                    if( cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0 )
                        vec->push_back(Rect(cvRound(x*factor), cvRound(y*factor),
                                            winSize.width, winSize.height)); 
                }
    }
    
    const CvHaarClassifierCascade* cascade;
    int stripSize;
    double factor;
    Mat sum1, sqsum1, *norm1, *mask1;
    Rect equRect;
    ConcurrentRectVector* vec;
};

struct HaarDetectObjects_ScaleCascade_Invoker
{
    HaarDetectObjects_ScaleCascade_Invoker( const CvHaarClassifierCascade* _cascade,
                                            Size _winsize, const Range& _xrange, double _ystep,
                                            size_t _sumstep, const int** _p, const int** _pq,
                                            ConcurrentRectVector& _vec )
    {
        cascade = _cascade;
        winsize = _winsize;
        xrange = _xrange;
        ystep = _ystep;
        sumstep = _sumstep;
        p = _p; pq = _pq;
        vec = &_vec;
    }
    
    void operator()( const BlockedRange& range ) const
    {
        int iy, startY = range.begin(), endY = range.end();
        const int *p0 = p[0], *p1 = p[1], *p2 = p[2], *p3 = p[3];
        const int *pq0 = pq[0], *pq1 = pq[1], *pq2 = pq[2], *pq3 = pq[3];
        bool doCannyPruning = p0 != 0;
        int sstep = sumstep/sizeof(p0[0]);
        
        for( iy = startY; iy < endY; iy++ )
        {
            int ix, y = cvRound(iy*ystep), ixstep = 1;
            for( ix = xrange.start; ix < xrange.end; ix += ixstep )
            {
                int x = cvRound(ix*ystep); // it should really be ystep, not ixstep
                
                if( doCannyPruning )
                {
                    int offset = y*sstep + x;
                    int s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
                    int sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
                    if( s < 100 || sq < 20 )
                    {
                        ixstep = 2;
                        continue;
                    }
                }
                
                int result = cvRunHaarClassifierCascade( cascade, cvPoint(x, y), 0 );
                if( result > 0 )
                    vec->push_back(Rect(x, y, winsize.width, winsize.height));
                ixstep = result != 0 ? 1 : 2;
            }
        }
    }
    
    const CvHaarClassifierCascade* cascade;
    double ystep;
    size_t sumstep;
    Size winsize;
    Range xrange;
    const int** p;
    const int** pq;
    ConcurrentRectVector* vec;
};
    
    
}


/* create more efficient internal representation of haar classifier cascade */
static CvHidHaarClassifierCascade*
icvCreateHidHaarClassifierCascade( CvHaarClassifierCascade* cascade )
{
    CvRect* ipp_features = 0;
    float *ipp_weights = 0, *ipp_thresholds = 0, *ipp_val1 = 0, *ipp_val2 = 0;
    int* ipp_counts = 0;

    CvHidHaarClassifierCascade* out = 0;

    int i, j, k, l;
    int datasize;
    int total_classifiers = 0;
    int total_nodes = 0;
    char errorstr[100];
    CvHidHaarClassifier* haar_classifier_ptr;
    CvHidHaarTreeNode* haar_node_ptr;
    CvSize orig_window_size;
    int has_tilted_features = 0;
    int max_count = 0;

    if( !CV_IS_HAAR_CLASSIFIER(cascade) )
        CV_Error( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier pointer" );

    if( cascade->hid_cascade )
        CV_Error( CV_StsError, "hid_cascade has been already created" );

    if( !cascade->stage_classifier )
        CV_Error( CV_StsNullPtr, "" );

    if( cascade->count <= 0 )
        CV_Error( CV_StsOutOfRange, "Negative number of cascade stages" );

    orig_window_size = cascade->orig_window_size;

    /* check input structure correctness and calculate total memory size needed for
       internal representation of the classifier cascade */
    for( i = 0; i < cascade->count; i++ )
    {
        CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;

        if( !stage_classifier->classifier ||
            stage_classifier->count <= 0 )
        {
            sprintf( errorstr, "header of the stage classifier #%d is invalid "
                     "(has null pointers or non-positive classfier count)", i );
            CV_Error( CV_StsError, errorstr );
        }

        max_count = MAX( max_count, stage_classifier->count );
        total_classifiers += stage_classifier->count;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            CvHaarClassifier* classifier = stage_classifier->classifier + j;

            total_nodes += classifier->count;
            for( l = 0; l < classifier->count; l++ )
            {
                for( k = 0; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    if( classifier->haar_feature[l].rect[k].r.width )
                    {
                        CvRect r = classifier->haar_feature[l].rect[k].r;
                        int tilted = classifier->haar_feature[l].tilted;
                        has_tilted_features |= tilted != 0;
                        if( r.width < 0 || r.height < 0 || r.y < 0 ||
                            r.x + r.width > orig_window_size.width
                            ||
                            (!tilted &&
                            (r.x < 0 || r.y + r.height > orig_window_size.height))
                            ||
                            (tilted && (r.x - r.height < 0 ||
                            r.y + r.width + r.height > orig_window_size.height)))
                        {
                            sprintf( errorstr, "rectangle #%d of the classifier #%d of "
                                     "the stage classifier #%d is not inside "
                                     "the reference (original) cascade window", k, j, i );
                            CV_Error( CV_StsNullPtr, errorstr );
                        }
                    }
                }
            }
        }
    }

    // this is an upper boundary for the whole hidden cascade size
    datasize = sizeof(CvHidHaarClassifierCascade) +
               sizeof(CvHidHaarStageClassifier)*cascade->count +
               sizeof(CvHidHaarClassifier) * total_classifiers +
               sizeof(CvHidHaarTreeNode) * total_nodes +
               sizeof(void*)*(total_nodes + total_classifiers);

    out = (CvHidHaarClassifierCascade*)cvAlloc( datasize );
    memset( out, 0, sizeof(*out) );

    /* init header */
    out->count = cascade->count;
    out->stage_classifier = (CvHidHaarStageClassifier*)(out + 1);
    haar_classifier_ptr = (CvHidHaarClassifier*)(out->stage_classifier + cascade->count);
    haar_node_ptr = (CvHidHaarTreeNode*)(haar_classifier_ptr + total_classifiers);

    out->is_stump_based = 1;
    out->has_tilted_features = has_tilted_features;
    out->is_tree = 0;

    /* initialize internal representation */
    for( i = 0; i < cascade->count; i++ )
    {
        CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;
        CvHidHaarStageClassifier* hid_stage_classifier = out->stage_classifier + i;

        hid_stage_classifier->count = stage_classifier->count;
        hid_stage_classifier->threshold = stage_classifier->threshold - icv_stage_threshold_bias;
        hid_stage_classifier->classifier = haar_classifier_ptr;
        hid_stage_classifier->two_rects = 1;
        haar_classifier_ptr += stage_classifier->count;

        hid_stage_classifier->parent = (stage_classifier->parent == -1)
            ? NULL : out->stage_classifier + stage_classifier->parent;
        hid_stage_classifier->next = (stage_classifier->next == -1)
            ? NULL : out->stage_classifier + stage_classifier->next;
        hid_stage_classifier->child = (stage_classifier->child == -1)
            ? NULL : out->stage_classifier + stage_classifier->child;

        out->is_tree |= hid_stage_classifier->next != NULL;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            CvHaarClassifier* classifier = stage_classifier->classifier + j;
            CvHidHaarClassifier* hid_classifier = hid_stage_classifier->classifier + j;
            int node_count = classifier->count;
            float* alpha_ptr = (float*)(haar_node_ptr + node_count);

            hid_classifier->count = node_count;
            hid_classifier->node = haar_node_ptr;
            hid_classifier->alpha = alpha_ptr;

            for( l = 0; l < node_count; l++ )
            {
                CvHidHaarTreeNode* node = hid_classifier->node + l;
                CvHaarFeature* feature = classifier->haar_feature + l;
                memset( node, -1, sizeof(*node) );
                node->threshold = classifier->threshold[l];
                node->left = classifier->left[l];
                node->right = classifier->right[l];

                if( fabs(feature->rect[2].weight) < DBL_EPSILON ||
                    feature->rect[2].r.width == 0 ||
                    feature->rect[2].r.height == 0 )
                    memset( &(node->feature.rect[2]), 0, sizeof(node->feature.rect[2]) );
                else
                    hid_stage_classifier->two_rects = 0;
            }

            memcpy( alpha_ptr, classifier->alpha, (node_count+1)*sizeof(alpha_ptr[0]));
            haar_node_ptr =
                (CvHidHaarTreeNode*)cvAlignPtr(alpha_ptr+node_count+1, sizeof(void*));

            out->is_stump_based &= node_count == 1;
        }
    }

#ifdef HAVE_IPP
    int can_use_ipp = !out->has_tilted_features && !out->is_tree && out->is_stump_based;

    if( can_use_ipp )
    {
        int ipp_datasize = cascade->count*sizeof(out->ipp_stages[0]);
        float ipp_weight_scale=(float)(1./((orig_window_size.width-icv_object_win_border*2)*
            (orig_window_size.height-icv_object_win_border*2)));

        out->ipp_stages = (void**)cvAlloc( ipp_datasize );
        memset( out->ipp_stages, 0, ipp_datasize );

        ipp_features = (CvRect*)cvAlloc( max_count*3*sizeof(ipp_features[0]) );
        ipp_weights = (float*)cvAlloc( max_count*3*sizeof(ipp_weights[0]) );
        ipp_thresholds = (float*)cvAlloc( max_count*sizeof(ipp_thresholds[0]) );
        ipp_val1 = (float*)cvAlloc( max_count*sizeof(ipp_val1[0]) );
        ipp_val2 = (float*)cvAlloc( max_count*sizeof(ipp_val2[0]) );
        ipp_counts = (int*)cvAlloc( max_count*sizeof(ipp_counts[0]) );

        for( i = 0; i < cascade->count; i++ )
        {
            CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;
            for( j = 0, k = 0; j < stage_classifier->count; j++ )
            {
                CvHaarClassifier* classifier = stage_classifier->classifier + j;
                int rect_count = 2 + (classifier->haar_feature->rect[2].r.width != 0);

                ipp_thresholds[j] = classifier->threshold[0];
                ipp_val1[j] = classifier->alpha[0];
                ipp_val2[j] = classifier->alpha[1];
                ipp_counts[j] = rect_count;

                for( l = 0; l < rect_count; l++, k++ )
                {
                    ipp_features[k] = classifier->haar_feature->rect[l].r;
                    //ipp_features[k].y = orig_window_size.height - ipp_features[k].y - ipp_features[k].height;
                    ipp_weights[k] = classifier->haar_feature->rect[l].weight*ipp_weight_scale;
                }
            }

            if( ippiHaarClassifierInitAlloc_32f( (IppiHaarClassifier_32f**)&out->ipp_stages[i],
                (const IppiRect*)ipp_features, ipp_weights, ipp_thresholds,
                ipp_val1, ipp_val2, ipp_counts, stage_classifier->count ) < 0 )
                break;
        }

        if( i < cascade->count )
        {
            for( j = 0; j < i; j++ )
                if( out->ipp_stages[i] )
                    ippiHaarClassifierFree_32f( (IppiHaarClassifier_32f*)out->ipp_stages[i] );
            cvFree( &out->ipp_stages );
        }
    }
#endif

    cascade->hid_cascade = out;
    assert( (char*)haar_node_ptr - (char*)out <= datasize );

    cvFree( &ipp_features );
    cvFree( &ipp_weights );
    cvFree( &ipp_thresholds );
    cvFree( &ipp_val1 );
    cvFree( &ipp_val2 );
    cvFree( &ipp_counts );

    return out;
}


CV_IMPL CvSeq*
myCvHaarDetectObjects( const CvArr* _img,
                     CvHaarClassifierCascade* cascade,
                     CvMemStorage* storage, double scaleFactor,
                     int minNeighbors, int flags, CvSize minSize )
{
    const double GROUP_EPS = 0.2;
    CvMat stub, *img = (CvMat*)_img;
    cv::Ptr<CvMat> temp, sum, tilted, sqsum, normImg, sumcanny, imgSmall;
    CvSeq* result_seq = 0;
    cv::Ptr<CvMemStorage> temp_storage;

    cv::ConcurrentRectVector allCandidates;
    std::vector<cv::Rect> rectList;
    std::vector<int> rweights;
    double factor;
    int coi;
    bool doCannyPruning = (flags & CV_HAAR_DO_CANNY_PRUNING) != 0;
    bool findBiggestObject = (flags & CV_HAAR_FIND_BIGGEST_OBJECT) != 0;
    bool roughSearch = (flags & CV_HAAR_DO_ROUGH_SEARCH) != 0;

    if( !CV_IS_HAAR_CLASSIFIER(cascade) )
        CV_Error( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

    if( !storage )
        CV_Error( CV_StsNullPtr, "Null storage pointer" );

    img = cvGetMat( img, &stub, &coi );
    if( coi )
        CV_Error( CV_BadCOI, "COI is not supported" );

    if( CV_MAT_DEPTH(img->type) != CV_8U )
        CV_Error( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );
    
    if( scaleFactor <= 1 )
        CV_Error( CV_StsOutOfRange, "scale factor must be > 1" );

    if( findBiggestObject )
        flags &= ~CV_HAAR_SCALE_IMAGE;

    temp = cvCreateMat( img->rows, img->cols, CV_8UC1 );
    sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
    sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 );

    if( !cascade->hid_cascade )
        icvCreateHidHaarClassifierCascade(cascade);

    if( cascade->hid_cascade->has_tilted_features )
        tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

    result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

    if( CV_MAT_CN(img->type) > 1 )
    {
        cvCvtColor( img, temp, CV_BGR2GRAY );
        img = temp;
    }

    if( findBiggestObject )
        flags &= ~(CV_HAAR_SCALE_IMAGE|CV_HAAR_DO_CANNY_PRUNING);

    if( flags & CV_HAAR_SCALE_IMAGE )
    {
        CvSize winSize0 = cascade->orig_window_size;
#ifdef HAVE_IPP
        int use_ipp = cascade->hid_cascade->ipp_stages != 0;

        if( use_ipp )
            normImg = cvCreateMat( img->rows, img->cols, CV_32FC1 );
#endif
        imgSmall = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 );

        for( factor = 1; ; factor *= scaleFactor )
        {
            CvSize winSize = { cvRound(winSize0.width*factor),
                                cvRound(winSize0.height*factor) };
            CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
            CvSize sz1 = { sz.width - winSize0.width, sz.height - winSize0.height };

            CvRect equRect = { icv_object_win_border, icv_object_win_border,
                winSize0.width - icv_object_win_border*2,
                winSize0.height - icv_object_win_border*2 };

            CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
            CvMat* _tilted = 0;

            if( sz1.width <= 0 || sz1.height <= 0 )
                break;
            if( winSize.width < minSize.width || winSize.height < minSize.height )
                continue;

            img1 = cvMat( sz.height, sz.width, CV_8UC1, imgSmall->data.ptr );
            sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
            sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
            if( tilted )
            {
                tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
                _tilted = &tilted1;
            }
            norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, normImg ? normImg->data.ptr : 0 );
            mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );

            cvResize( img, &img1, CV_INTER_LINEAR );
            cvIntegral( &img1, &sum1, &sqsum1, _tilted );

            int ystep = factor > 2 ? 1 : 2;
        #ifdef HAVE_TBB
            const int LOCS_PER_THREAD = 1000;
            int stripCount = ((sz1.width/ystep)*(sz1.height + ystep-1)/ystep + LOCS_PER_THREAD/2)/LOCS_PER_THREAD;
            stripCount = std::min(std::max(stripCount, 1), 100);
        #else
            const int stripCount = 1;
        #endif
            
#ifdef HAVE_IPP
            if( use_ipp )
            {
                cv::Mat fsum(sum1.rows, sum1.cols, CV_32F, sum1.data.ptr, sum1.step);
                cv::Mat(&sum1).convertTo(fsum, CV_32F, 1, -(1<<24));
            }
            else
#endif
                cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, _tilted, 1. );            
            
            cv::Mat _norm1(&norm1), _mask1(&mask1);
            cv::parallel_for(cv::BlockedRange(0, stripCount),
                         cv::HaarDetectObjects_ScaleImage_Invoker(cascade,
                                (((sz1.height + stripCount - 1)/stripCount + ystep-1)/ystep)*ystep,
                                factor, cv::Mat(&sum1), cv::Mat(&sqsum1), &_norm1, &_mask1,
                                cv::Rect(equRect), allCandidates));
        }
    }
    else
    {
        int n_factors = 0;
        cv::Rect scanROI;

        cvIntegral( img, sum, sqsum, tilted );

        if( doCannyPruning )
        {
            sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
            cvCanny( img, temp, 0, 50, 3 );
            cvIntegral( temp, sumcanny );
        }

        for( n_factors = 0, factor = 1;
             factor*cascade->orig_window_size.width < img->cols - 10 &&
             factor*cascade->orig_window_size.height < img->rows - 10;
             n_factors++, factor *= scaleFactor )
            ;

        if( findBiggestObject )
        {
            scaleFactor = 1./scaleFactor;
            factor *= scaleFactor;
        }
        else
            factor = 1;

        for( ; n_factors-- > 0; factor *= scaleFactor )
        {
            const double ystep = std::max( 2., factor );
            CvSize winSize = { cvRound( cascade->orig_window_size.width * factor ),
                                cvRound( cascade->orig_window_size.height * factor )};
            CvRect equRect = { 0, 0, 0, 0 };
            int *p[4] = {0,0,0,0};
            int *pq[4] = {0,0,0,0};
            int startX = 0, startY = 0;
            int endX = cvRound((img->cols - winSize.width) / ystep);
            int endY = cvRound((img->rows - winSize.height) / ystep);

            if( winSize.width < minSize.width || winSize.height < minSize.height )
            {
                if( findBiggestObject )
                    break;
                continue;
            }

            cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
            cvZero( temp );

            if( doCannyPruning )
            {
                equRect.x = cvRound(winSize.width*0.15);
                equRect.y = cvRound(winSize.height*0.15);
                equRect.width = cvRound(winSize.width*0.7);
                equRect.height = cvRound(winSize.height*0.7);

                p[0] = (int*)(sumcanny->data.ptr + equRect.y*sumcanny->step) + equRect.x;
                p[1] = (int*)(sumcanny->data.ptr + equRect.y*sumcanny->step)
                            + equRect.x + equRect.width;
                p[2] = (int*)(sumcanny->data.ptr + (equRect.y + equRect.height)*sumcanny->step) + equRect.x;
                p[3] = (int*)(sumcanny->data.ptr + (equRect.y + equRect.height)*sumcanny->step)
                            + equRect.x + equRect.width;

                pq[0] = (int*)(sum->data.ptr + equRect.y*sum->step) + equRect.x;
                pq[1] = (int*)(sum->data.ptr + equRect.y*sum->step)
                            + equRect.x + equRect.width;
                pq[2] = (int*)(sum->data.ptr + (equRect.y + equRect.height)*sum->step) + equRect.x;
                pq[3] = (int*)(sum->data.ptr + (equRect.y + equRect.height)*sum->step)
                            + equRect.x + equRect.width;
            }

            if( scanROI.area() > 0 )
            {
                //adjust start_height and stop_height
                startY = cvRound(scanROI.y / ystep);
                endY = cvRound((scanROI.y + scanROI.height - winSize.height) / ystep);

                startX = cvRound(scanROI.x / ystep);
                endX = cvRound((scanROI.x + scanROI.width - winSize.width) / ystep);
            }

            cv::parallel_for(cv::BlockedRange(startY, endY),
                cv::HaarDetectObjects_ScaleCascade_Invoker(cascade, winSize, cv::Range(startX, endX),
                                                           ystep, sum->step, (const int**)p,
                                                           (const int**)pq, allCandidates ));

            if( findBiggestObject && !allCandidates.empty() && scanROI.area() == 0 )
            {
                rectList.resize(allCandidates.size());
                std::copy(allCandidates.begin(), allCandidates.end(), rectList.begin());
                
                groupRectangles(rectList, std::max(minNeighbors, 1), GROUP_EPS);
                
                if( !rectList.empty() )
                {
                    size_t i, sz = rectList.size();
                    cv::Rect maxRect;
                    
                    for( i = 0; i < sz; i++ )
                    {
                        if( rectList[i].area() > maxRect.area() )
                            maxRect = rectList[i];
                    }
                    
                    allCandidates.push_back(maxRect);
                    
                    scanROI = maxRect;
                    int dx = cvRound(maxRect.width*GROUP_EPS);
                    int dy = cvRound(maxRect.height*GROUP_EPS);
                    scanROI.x = std::max(scanROI.x - dx, 0);
                    scanROI.y = std::max(scanROI.y - dy, 0);
                    scanROI.width = std::min(scanROI.width + dx*2, img->cols-1-scanROI.x);
                    scanROI.height = std::min(scanROI.height + dy*2, img->rows-1-scanROI.y);
                
                    double minScale = roughSearch ? 0.6 : 0.4;
                    minSize.width = cvRound(maxRect.width*minScale);
                    minSize.height = cvRound(maxRect.height*minScale);
                }
            }
        }
    }

    rectList.resize(allCandidates.size());
    if(!allCandidates.empty())
        std::copy(allCandidates.begin(), allCandidates.end(), rectList.begin());
    
    if( minNeighbors != 0 || findBiggestObject )
        groupRectangles(rectList, rweights, std::max(minNeighbors, 1), GROUP_EPS);
        
//     if( findBiggestObject && rectList.size() )
//     {
//         CvAvgComp result_comp = {{0,0,0,0},0};
//         
//         for( size_t i = 0; i < rectList.size(); i++ )
//         {
//             cv::Rect r = rectList[i];
//             if( r.area() > cv::Rect(result_comp.rect).area() )
//             {
//                 result_comp.rect = r;
//                 result_comp.neighbors = rweights[i];
//             }
//         }
//         cvSeqPush( result_seq, &result_comp );
//     }
//     else
//     {
        for( size_t i = 0; i < rectList.size(); i++ )
        {
            CvAvgComp c;
            c.rect = rectList[i];
            c.neighbors = i < rweights.size() ? rweights[i] : 0;
            cvSeqPush( result_seq, &c );
        }
    //}

    return result_seq;
}


EyeCounter::EyeCounter(QObject *parent, QString faceXml, QString eyeXml)
	: QObject(parent)
{
	QFileInfo info = QFileInfo(eyeXml);
	if(!info.exists())
	{
		eyeXml = tr("%1/%2").arg(OPENCV_DATA_DIR).arg(eyeXml);
		info = QFileInfo(eyeXml);
		if(!info.exists())
			qDebug() << "File:"<<eyeXml<<" doesnt exist!";
	}
	
	/* load the face classifier */
	QStringList files = QStringList() 
		<< "haarcascade_frontalface_alt.xml"
		<< "haarcascade_frontalface_alt2.xml"
		<< "haarcascade_frontalface_alt_tree.xml"
		<< "haarcascade_frontalface_default.xml"
		<< "haarcascade_profileface.xml";
	foreach(QString file, files)
	{
		CvHaarClassifierCascade *cascade = (CvHaarClassifierCascade*)cvLoad(qPrintable(tr("%1/%2").arg(OPENCV_DATA_DIR).arg(file)), 0, 0, 0);
		if(cascade)
			m_cvCascadeFaces << cascade;
		else
			qDebug() << "Error loading "<<file<<", skipping"; 
	}
	
	/* load the eye classifier */
	m_cvCascadeEyes = (CvHaarClassifierCascade*)cvLoad(qPrintable(eyeXml), 0, 0, 0);
	
	/* setup memory storage, needed by the object detector */
	m_cvStorage = cvCreateMemStorage(0);
}

EyeCounter::~EyeCounter()
{
// 	delete m_cvCascadeFace;
// 	delete m_cvCascadeEyes;
// 	delete m_cvStorage;
	cvReleaseMemStorage(&m_cvStorage);
}

QList<EyeCounterResult> EyeCounter::detectEyes(QImage image, bool includeFacesWithNoEyes)
{
	//return QList<EyeCounterResult>();
	
	static CvRect prev;
	static IplImage *gray, *small_img;
	double scale = 2.5;
	
	//image = image.scaled(320,240);
	
	if(image.format() != QImage::Format_ARGB32)
		image = image.convertToFormat(QImage::Format_ARGB32);
		
	IplImage *img = cvCreateImageHeader( cvSize(image.width(), image.height()), IPL_DEPTH_8U, 4);
	img->imageData = (char*) image.bits();
	
 
	if(!gray) {
		gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
		small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
						   cvRound (img->height/scale)), 8, 1 );
	}
	
	bool converted = false;
	
	
	#define PAD_FACE 2
	#define PAD_FACE_2 4
	QList<EyeCounterResult> faceList;
	
	
	int reloopCount = 0;
	while(reloopCount < 1)
	{
		if(prev.width > 0 && prev.height > 0) 
		{
			cvSetImageROI(small_img, prev);
		
			CvRect tPrev = cvRect(prev.x * scale, prev.y * scale, prev.width * scale, prev.height * scale);
			cvSetImageROI(img, tPrev);
			cvSetImageROI(gray, tPrev);
			
		} 
		else 
		{
			//printf("Resetting ROI\n");
			cvResetImageROI(img);
			cvResetImageROI(small_img);
			cvResetImageROI(gray);
		}
		
		if(!converted)
		{
			cvCvtColor( img, gray, CV_BGR2GRAY );
			cvResize( gray, small_img, CV_INTER_LINEAR );
			cvEqualizeHist( small_img, small_img );
			//converted = true;

		}
		
		cvClearMemStorage( m_cvStorage );
		
		QTime t;
		t.start();
		CvSeq* faces = myCvHaarDetectObjects( small_img, m_cvCascadeFaces[0], m_cvStorage,
						1.4, 0, 0
						|CV_HAAR_FIND_BIGGEST_OBJECT
						|CV_HAAR_DO_ROUGH_SEARCH
						//|CV_HAAR_DO_CANNY_PRUNING
						//|CV_HAAR_SCALE_IMAGE
						,
						cvSize(30, 30) );
		
		int ms = t.elapsed();
		QRect foundFace;
		
		qDebug() << "Debug: cvHarr detected"<<faces->total<<"faces in"<<ms<<"ms";
		if(faces->total > 0)
		{
			//faces->total = 1;
			
			QRect searchRect;
			for(int faceNum=0; faceNum<faces->total; faceNum++)
			{
				CvRect* r = (CvRect*)cvGetSeqElem( faces, faceNum );
				
				int startX,startY;
				if(prev.width > 0 && prev.height > 0) 
				{
					r->x += prev.x;
					r->y += prev.y;
				}
				startX = MAX(r->x - PAD_FACE,0);
				startY = MAX(r->y - PAD_FACE,0);
				
				int w = small_img->width  - startX - r->width  - PAD_FACE_2;
				int h = small_img->height - startY - r->height - PAD_FACE_2;
				int sw = r->x - PAD_FACE, 
				    sh = r->y - PAD_FACE;
				
				QRect newSearch(startX, startY,
					r->width  + PAD_FACE_2 + ((w < 0) ? w : 0) + ((sw < 0) ? sw : 0),
					r->height + PAD_FACE_2 + ((h < 0) ? h : 0) + ((sh < 0) ? sh : 0));
				
				if(searchRect.isNull())
					searchRect = newSearch;
				else
					searchRect = searchRect.united(newSearch);
				
				//printf("found face (%d,%d,%d,%d) setting ROI to (%d,%d,%d,%d)\n",r->x,r->y,r->width,r->height,prev.x,prev.y,prev.width,prev.height);
				
				int fx = (int)((double)r->x * scale);
				int fy = (int)((double)r->y * scale);
				int fw = (int)((double)r->width * scale);
				int fh = (int)((double)r->height * scale);
				foundFace = QRect(fx,fy,fw,fh);
				
				EyeCounterResult person;
				person.face = foundFace; 
				faceList << person;
			}
			
			//searchRect = searchRect.adjusted(-5,-5,10,10);
			prev = cvRect(searchRect.x(), searchRect.y(), searchRect.width(), searchRect.height());
					
			break;
			
		} 
		else 
		{
			prev.width = prev.height = 0;
			//foundFace = QRect();
			//qDebug() << "No face found";
			reloopCount ++;
		}
		
		
// 		if(faces && faces->total>0) 
// 		{
// 			CvRect* r = (CvRect*)cvGetSeqElem( faces, 0 );
// 			
// 			int startX,startY;
// 			if(prev.width > 0 && prev.height > 0) 
// 			{
// 				r->x += prev.x;
// 				r->y += prev.y;
// 			}
// 			startX = MAX(r->x - PAD_FACE,0);
// 			startY = MAX(r->y - PAD_FACE,0);
// 			
// 			int w = small_img->width - startX - r->width - PAD_FACE_2;
// 			int h = small_img->height - startY - r->height - PAD_FACE_2;
// 			int sw = r->x - PAD_FACE, sh = r->y - PAD_FACE;
// 			
// 			prev = cvRect(startX, startY,
// 				r->width  + PAD_FACE_2 + ((w < 0) ? w : 0) + ((sw < 0) ? sw : 0),
// 				r->height + PAD_FACE_2 + ((h < 0) ? h : 0) + ((sh < 0) ? sh : 0));
// 			
// 			//printf("found face (%d,%d,%d,%d) setting ROI to (%d,%d,%d,%d)\n",r->x,r->y,r->width,r->height,prev.x,prev.y,prev.width,prev.height);
// 			
// 			int fx = (int)((double)r->x * scale);
// 			int fy = (int)((double)r->y * scale);
// 			int fw = (int)((double)r->width * scale);
// 			int fh = (int)((double)r->height * scale);
// 			foundFace = QRect(fx,fy,fw,fh);
// 			
// 			EyeCounterResult person;
// 			person.face = foundFace; 
// 			faceList << person;
// 			break;
// 			
// 		} 
// 		else 
// 		{
// 			prev.width = prev.height = 0;
// 			//foundFace = QRect();
// 			//qDebug() << "No face found";
// 			reloopCount ++;
// 		}
	}
	
	cvReleaseImage(&img);
	
	return faceList;
	
#if 0
	qDebug() << "EyeCounter::detectEyes: image size:"<<image.size();
	IplImage *cvImg;
	//cvImg = QImage2IplImage(image); //.scaled(1024,1024));
	
	image = image.scaled(320,240);
	if(image.format() != QImage::Format_ARGB32)
		image = image.convertToFormat(QImage::Format_ARGB32);
		
	cvImg = cvCreateImageHeader( cvSize(image.width(), image.height()), IPL_DEPTH_8U, 4);
	cvImg->imageData = (char*) image.bits();
	
// 	uchar* newdata = (uchar*) malloc(sizeof(uchar) * image.byteCount());
// 	memcpy(newdata, image.bits(), image.byteCount());
// 	imgHeader->imageData = (char*)newdata;
	
	/* always check */
	//assert(cascade_f && cascade_e && storage && img);
	
	//m_cvStorage = cvCreateMemStorage(0);
	
	
	int i;

	/* detect faces */
	CvSeq *faces;
	foreach(CvHaarClassifierCascade *cascade, m_cvCascadeFaces)
	{
		m_cvStorage = cvCreateMemStorage(0);
		
		faces = cvHaarDetectObjects(
			cvImg, 
			cascade, 
			m_cvStorage,
			1.1, 2, CV_HAAR_DO_CANNY_PRUNING, 
			cvSize( 40, 40 ) );
		
		cvClearMemStorage(m_cvStorage);
		cvReleaseMemStorage(&m_cvStorage);
		
		if(faces->total > 0)
			break;
			
		if ( faces != NULL )
			cvClearSeq( faces );
		//cvReleaseSeq( &faces );
	}
	 

	QList<EyeCounterResult> faceList;
	
	qDebug() << "EyeCounter::detectEyes: faces->total:"<<faces->total;
	
	for(int faceNum = 0; faceNum < MIN(5, faces->total); faceNum ++)
	{
		qDebug() << "EyeCounter::detectEyes: processing face #" << faceNum;
		/* draw a rectangle */
		CvRect *faceRect = (CvRect*)cvGetSeqElem(faces, faceNum);
					
	// 	cvRectangle(cvImg,
	// 		cvPoint(r->x, r->y),
	// 		cvPoint(r->x + r->width, r->y + r->height),
	// 		CV_RGB(255, 0, 0), 1, 8, 0);
	
		/* reset buffer for the next object detection */
		
	
		/* Set the Region of Interest: estimate the eyes' position */
		int fx = faceRect->x, 
		    fy = (int)(faceRect->y + (faceRect->height/5.5));
		
		int fw = faceRect->width;
		int fh = (int)(faceRect->height/3.0);
		
		int fx2 = fx + fw;
		int fy2 = fy + fh;
		
// 		rect.width >= 0 && 
// 		rect.height >= 0 && 
// 		rect.x < image->width && 
// 		rect.y < image->height && 
// 		rect.x + rect.width >= (int)(rect.width > 0) && 
// 		rect.y + rect.height >= (int)(rect.height > 0)
		
		if(fw < 0 || fh < 0 ||
		   fx > cvImg->width || 
		   fy > cvImg->height)
		{
		 
			qDebug() << "ROI Error on face "<<faceNum<<", skipping";
			continue;
		}
		 
// 		cvSetImageROI(cvImg, 
// 			cvRect(fx, fy, 
// 			       fw, fh
// 			      ));
// 	
// 		/* detect eyes */
// 		CvSeq* eyes = cvHaarDetectObjects( 
// 			cvImg, 
// 			m_cvCascadeEyes, 
// 			m_cvStorage,
// 			1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
// 			cvSize(40, 30));
// 		cvClearMemStorage(m_cvStorage);
// 	
// 	
// 		QList<QRect> eyeRects;
// 		
// 		qDebug() << "EyeCounter::detectEyes: face "<<faceNum<<", eyes->total:"<<eyes->total;
// 	
// 		/* draw a rectangle for each eye found */
// 		for( i = 0; i < (eyes ? MIN(2, eyes->total) : 0); i++ ) 
// 		{
// 			CvRect *r = (CvRect*)cvGetSeqElem( eyes, i );
// 			eyeRects << QRect(r->x + fx, r->y + fy, r->width, r->height);
// 	// 		cvRectangle(img, 
// 	// 			cvPoint(r->x, r->y), 
// 	// 			cvPoint(r->x + r->width, r->y + r->height),
// 	// 			CV_RGB(255, 0, 0), 1, 8, 0);
// 		}
// 		
// 		if(!includeFacesWithNoEyes && eyeRects.isEmpty())
// 			continue;
			
		EyeCounterResult person;
		person.face = QRect(faceRect->x, faceRect->y, faceRect->width, faceRect->height);
		
// 		// Just guesing that the first and second rect is left/right eye!
// 		if(eyeRects.size() > 0)
// 			person.leftEye = eyeRects.at(0);
// 		if(eyeRects.size() > 1)
// 			person.rightEye = eyeRects.at(1);
// 	
// 		person.allEyes = eyeRects;
	
		faceList << person;
	
// 		cvResetImageROI(cvImg);
// 		if ( eyes != NULL )
// 			cvClearSeq( eyes  );
	}
		
	cvReleaseImage(&cvImg);
	if ( faces != NULL )
	{
		cvClearSeq( faces );
		//cvRelease((void**)faces);
	}
//	cvReleaseSeq( &faces );
	
	qDebug() << "EyeCounter::detectEyes: done processing, returning faceList";
	//cvReleaseMemStorage(&m_cvStorage);
	
	return faceList;
#endif
}



IplImage* QImage2IplImage(QImage qimg)
{
	if(qimg.format() != QImage::Format_ARGB32)
		qimg = qimg.convertToFormat(QImage::Format_ARGB32);
		
	IplImage *imgHeader = cvCreateImageHeader( cvSize(qimg.width(), qimg.width()), IPL_DEPTH_8U, 4);
	imgHeader->imageData = (char*) qimg.bits();
	
	uchar* newdata = (uchar*) malloc(sizeof(uchar) * qimg.byteCount());
	memcpy(newdata, qimg.bits(), qimg.byteCount());
	imgHeader->imageData = (char*)newdata;
	
	// NB: Caller is responsible for deleting imageData!
	return imgHeader;
}

QImage IplImage2QImage(IplImage *iplImg)
{
	int h        = iplImg->height;
	int w        = iplImg->width;
	int step     = iplImg->widthStep;
	char *data   = iplImg->imageData;
	int channels = iplImg->nChannels;
	
	QImage qimg(w, h, QImage::Format_ARGB32);
	for (int y = 0; y < h; y++, data += step)
	{
		for (int x = 0; x < w; x++)
		{
			char r=0, g=0, b=0, a=0;
			if (channels == 1)
			{
				r = data[x * channels];
				g = data[x * channels];
				b = data[x * channels];
			}
			else if (channels == 3 || channels == 4)
			{
				r = data[x * channels + 2];
				g = data[x * channels + 1];
				b = data[x * channels];
			}
			
			if (channels == 4)
			{
				a = data[x * channels + 3];
				qimg.setPixel(x, y, qRgba(r, g, b, a));
			}
			else
			{
				qimg.setPixel(x, y, qRgb(r, g, b));
			}
		}
	}
	return qimg;

}



