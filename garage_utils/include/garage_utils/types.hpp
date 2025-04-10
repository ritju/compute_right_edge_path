#ifndef GARAGE_UTILS__TYPES_HPP_
#define GARAGE_UTILS__TYPES_HPP_

#include <vector>
#include <cstdio>
#include <string>

namespace garage_utils_pkg
{
        // #define Point std::pair<double, double> // 加上#include "nav_msgs/msg/path.hpp"后编译报错a template-id may not appear in a using-declaration
        typedef std::pair<double, double> Point;  // 或 using Point = std::pair<double, double>;

        struct EnhancedPoint
        {
                int index;
                std::vector<int> adjacent_vec;
                bool visited;
                Point coord;
        };

        enum class NeighborType
        {
                POINT_POINT,
                POINT_LINE
        };

        // 顶点结构体（包含极角信息）
        struct Vertex 
        {
                double x, y;
                double angle; // 相对于矩形中心的极角
        };

        struct Edge
        {
                int index;
                int pt1_index;
                int pt2_index;
                Point pt1;
                Point pt2;
                double dis;
        };

        enum class SearchMethod
        {
                DepthFirstSearch
        };



        // 矩形结构体（增强版）
        struct EnhancedRect 
        {
        std::vector<Vertex> vertices;                             // 顶点集合（需排序后使用）
        std::vector<std::pair<size_t, size_t>> edges;             // 边集合（存储顶点索引）
        std::vector<size_t> adjacent_rects;                       // 相邻矩形索引列表
        size_t adjacent_rect_selected;                            // 最终选择的下个矩形索引值， -1代表最后一个
        bool visited = false;                                     // 遍历标记
        size_t index;                                             // 矩形索引值，用于调试矩形排序结果
        std::vector<std::pair<double, double>> middle_long_line;  // 矩形两条长边决定的遍历过程要覆盖的路径
        };

        void assert_(bool condition, std::string error_msg)
        {
                if (!condition)
                {
                        throw error_msg;
                }
        }


} // end of namespace

#endif

