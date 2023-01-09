#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    model << (float)cos(rotation_angle/180 * MY_PI), -(float)sin(rotation_angle/180 * MY_PI), 0, 0,
            (float)sin(rotation_angle/180 * MY_PI), (float)cos(rotation_angle/180 * MY_PI), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.

    float top = zNear * (float)tan(eye_fov/180 * MY_PI), bottom = -top;
    float right = aspect_ratio * top, left = -right;

    // Perspective to orthographic matrix
    Eigen::Matrix4f perspToOtrho = Eigen::Matrix4f::Identity();
    perspToOtrho << zNear, 0, 0, 0,
                  0, zNear, 0, 0,
                  0, 0, zNear+zFar, zNear*zFar,
                  0, 0, 1, 0;

    // orthographic projection matrix
    Eigen::Matrix4f ortho = Eigen::Matrix4f::Identity();
    ortho << 2/(right-left), 0, 0, -(left+right)/2,
             0, 2/(top-bottom), 0, -(bottom+top)/2,
             0, 0, 2/(zNear-zFar), -(zNear+zFar)/2,
             0, 0, 0, 1;

    projection = ortho * perspToOtrho;
    return projection;
}

Eigen::Matrix4f get_rotation(Vector3f axis, float angle) {
    // Tilde matrix 叉乘矩阵
    Eigen::Matrix3f tildeMatrix;
    tildeMatrix << 0, -axis.z(), axis.y(),
                   axis.z(), 0, -axis.x(),
                   -axis.y(), axis.x(), 0;

    // Use Rodrigues' Rotation Formula 罗德里格斯旋转公式
    Eigen::Matrix3f rotation;
    rotation = (float)cos(angle/180*MY_PI) * Eigen::Matrix3f::Identity() +
            (1 - (float)cos(angle/180*MY_PI)) * axis * axis.transpose() + (float)sin(angle/180*MY_PI) * tildeMatrix;

    // Linear to Affine
    Eigen::Matrix4f affineRotation;
    affineRotation << rotation(0, 0), rotation(0, 1), rotation(0, 2), 0,
                      rotation(1, 0), rotation(1, 1), rotation(01, 2), 0,
                      rotation(2, 0), rotation(2, 1), rotation(2, 2), 0,
                      0, 0, 0, 1;

    return affineRotation;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
