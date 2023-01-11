// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

static bool insideTriangle(float x, float y, const Triangle& t)
{
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    auto v = t.toVector4();

    // Cross product of vec(point2->pixel point) and vec(point2->point0)
    // Here we don't need z, cause we just put the point in a 2D plain, the depth is not necessary
    // From another aspect, we just use the z of cross product which is only relevant to x and y of the two initial vectors
    Vector3f firstCrossProduct  =
            (Vector3f(x, y, 0) - Vector3f(v[2].x(), v[2].y(), 0)).cross(
                    Vector3f(v[0].x(), v[0].y(), 0) - Vector3f(v[2].x(), v[2].y(), 0)
            );

    for(int i = 0; i < 2; i++) {
        Vector3f point1 = {v[i].x(), v[i].y(), 0};
        Vector3f point2 = {v[i+1].x(), v[i+1].y(), 0};
        Vector3f p = {x, y, 0};

        Vector3f currentCrossProduct = (p - point1).cross(point2 - point1);

        // if the product of two cross product.z() is minus, the point is not inside the Triangle, = 0 stands for the point is on the sides of the Triangle
        if(firstCrossProduct.z() * currentCrossProduct.z() < 0)
            return false;
    }
    return true;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    bool superSampling = false;

    auto v = t.toVector4();

    // TODO : Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle

    // get Bounding Box
    float xMin = v[0].x() < v[1].x() ? (v[0].x() < v[2].x() ? v[0].x() : v[2].x()) : (v[1].x() < v[2].x() ? v[1].x() : v[2].x());
    float xMax = v[0].x() > v[1].x() ? (v[0].x() > v[2].x() ? v[0].x()+1 : v[2].x()+1) : (v[1].x() > v[2].x() ? v[1].x()+1 : v[2].x()+1);
    float yMin = v[0].y() < v[1].y() ? (v[0].y() < v[2].y() ? v[0].y() : v[2].y()) : (v[1].y() < v[2].y() ? v[1].y() : v[2].y());
    float yMax = v[0].y() > v[1].y() ? (v[0].y() > v[2].y() ? v[0].y() : v[2].y()) : (v[1].y() > v[2].y() ? v[1].y()+1 : v[2].y()+1);

    // If so, use the following code to get the interpolated z value.
    // auto[alpha, beta, gamma] = computeBarycentric2D((float)pixelX, (float)pixelY, t.v);
    // float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
    // float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
    // z_interpolated *= w_reciprocal;

    // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.

    if(!superSampling) {
        // Rasterization
        for(int x = (int)xMin; x < (int)xMax; x++) {
            for(int y = (int)yMin; y < (int)yMax; y++) {
                if(insideTriangle((float)(x + 0.5), (float)(y + 0.5), t)) {
                    // interpolation
                    auto[alpha, beta, gamma] = computeBarycentric2D((float)x, (float)y, t.v);
                    float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    int buffer_index = get_index(x, y);
                    if(z_interpolated < depth_buf[buffer_index]) {  // if current depth is the min depth
                        set_pixel({(float)x, (float)y, 0}, t.getColor());
                        depth_buf[buffer_index] = z_interpolated;
                    }
                }
            }
        }
    }
    else {  // 2*2 Super Sampling Anti-Aliasing
        for(int x = (int)xMin; x < (int)xMax; x++) {
            for (int y = (int)yMin; y < (int)yMax; y++) {
                // the samplePoints[i].z() stands for the depth of the sample point,
                // samplePoints[i].w() stands for whether the sample point is inside the Triangle
                Vector4f samplePoints[4] = {
                        {(float)(x+0.25), (float)(y+0.25), 0, 0}, {(float)(x+0.25), (float)(y+0.75), 0, 0},
                        {(float)(x+0.75), (float)(y+0.25), 0, 0}, {(float)(x+0.75), (float)(y+0.75), 0, 0}
                };

                bool hasSamplePointInside = false;  // at least one sample point is inside the Triangle
                for(auto & samplePoint : samplePoints) {  // loop to judge whether the sample point is inside, and calculate depth as well
                    if(insideTriangle(samplePoint.x(), samplePoint.y(), t)) {
                        hasSamplePointInside = true;
                        samplePoint.w() = 1;
                    }
                    auto[alpha, beta, gamma] = computeBarycentric2D(samplePoint.x(), samplePoint.y(), t.v);
                    float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    samplePoint.z() = z_interpolated*w_reciprocal;
                }

                if(!hasSamplePointInside)  // the pixel is not inside the Triangle
                    continue;
                else {
                    int buffer_index = get_index(x, y), samplePointCount = 0;  // samplePointCount is the num of sample point which is inside and depth is less than the depth_buf
                    for(auto & samplePoint : samplePoints) {
                        if(samplePoint.z() < depth_buf[buffer_index] && samplePoint.w())
                            samplePointCount += 1;
                    }

                    float minDepth = std::min({samplePoints[0].z(), samplePoints[1].z(), samplePoints[2].z(), samplePoints[3].z()});
                    if(minDepth < depth_buf[buffer_index]) {
                        // the final color is a mix of t.getColor and the color in frame_buf
                        set_pixel({(float)x, (float)y, 0}, t.getColor() * samplePointCount / 4 + (4 - samplePointCount) * frame_buf[get_index(x, y)]);
                        depth_buf[get_index(x, y)] = minDepth;
                    }
                }
            }
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on