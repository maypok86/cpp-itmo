#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <vector>

enum class Axis
{
    X,
    Y
};

class Point
{
public:
    Point(const double x, const double y)
        : x_(x)
        , y_(y)
    {
    }

    double x() const { return x_; }
    double y() const { return y_; }
    double coord(Axis axis) const;
    double distance(const Point & that) const;

    bool operator<(const Point & that) const;
    bool operator>(const Point & that) const;
    bool operator<=(const Point & that) const;
    bool operator>=(const Point & that) const;
    bool operator==(const Point & that) const;
    bool operator!=(const Point & that) const;

    std::string to_string() const;

    friend std::ostream & operator<<(std::ostream &, const Point &);

private:
    double x_;
    double y_;
};

class Rect
{
public:
    Rect(const Point & left_bottom_, const Point & right_bottom_);

    double xmin() const { return left_bottom.x(); }
    double ymin() const { return left_bottom.y(); }
    double xmax() const { return right_top.x(); }
    double ymax() const { return right_top.y(); }
    double max_coord(Axis axis) const;
    double min_coord(Axis axis) const;
    double distance(const Point & p) const;

    bool contains(const Point & p) const;
    bool intersects(const Rect & rect) const;

private:
    const Point left_bottom;
    const Point right_top;
};

namespace rbtree {

class PointSet
{
public:
    class iterator
    {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = Point;
        using pointer = const Point *;
        using reference = const Point &;
        using iterator_category = std::forward_iterator_tag;

        iterator(const std::shared_ptr<std::vector<Point>> & vector_pointer_, const std::vector<Point>::iterator & current_);
        explicit iterator() = default;

        bool is_valid() const;

        reference operator*() const;

        pointer operator->() const;

        iterator & operator++();

        iterator operator++(int);

        bool operator==(const iterator & that) const;

        bool operator!=(const iterator & that) const;

    private:
        std::vector<Point>::iterator current;
        std::shared_ptr<std::vector<Point>> vector_pointer;
    };

    PointSet(const std::string & filename = {});

    bool empty() const;
    std::size_t size() const;
    void put(const Point & p);
    bool contains(const Point & p) const;

    // second iterator points to an element out of range
    std::pair<iterator, iterator> range(const Rect & rect) const;
    iterator begin() const;
    iterator end() const;

    std::optional<Point> nearest(const Point & p) const;
    // second iterator points to an element out of range
    std::pair<iterator, iterator> nearest(const Point & p, std::size_t k) const;

    friend std::ostream & operator<<(std::ostream & os, const PointSet & tree)
    {
        for (const auto & p : tree) {
            os << p << " ";
        }
        return os;
    }

private:
    std::shared_ptr<std::vector<Point>> points;
    std::set<Point> set_points;
};

} // namespace rbtree

namespace kdtree {

class PointSet
{
    struct Node
    {
        Node(const Point & p, const Rect & rect_, Axis axis_);

        bool operator<(const Point & that) const;
        bool operator==(const Point & that) const;
        bool operator!=(const Point & that) const;
        bool operator>(const Point & that) const;

        const Point point;
        const Rect rect;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        const Axis axis;
    };

    struct Distance
    {
        Distance(double distance_ = std::numeric_limits<double>::max(), const std::shared_ptr<Node> & node_ = nullptr)
            : distance(distance_)
            , node(node_)
        {
        }

        bool operator<(const Distance &) const;
        bool operator>(const Distance &) const;
        bool operator==(const Distance &) const;
        bool operator!=(const Distance &) const;
        bool operator>=(const Distance &) const;
        bool operator<=(const Distance &) const;

        double distance;
        std::shared_ptr<Node> node;
    };

public:
    class iterator
    {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = Point;
        using pointer = const Point *;
        using reference = const Point &;
        using iterator_category = std::forward_iterator_tag;

        iterator(const std::shared_ptr<std::vector<std::shared_ptr<Node>>> & vector_pointer_, const std::vector<std::shared_ptr<Node>>::iterator & current_);
        explicit iterator() = default;

        bool is_valid() const;

        reference operator*() const;

        pointer operator->() const;

        iterator & operator++();

        iterator operator++(int);

        bool operator==(const iterator &) const;

        bool operator!=(const iterator &) const;

    private:
        std::vector<std::shared_ptr<Node>>::iterator current;
        std::shared_ptr<std::vector<std::shared_ptr<Node>>> vector_pointer;
    };

    PointSet(const std::string & filename = {});

    bool empty() const
    {
        build_if_need();
        return points.empty();
    };
    std::size_t size() const
    {
        build_if_need();
        return points.size();
    };
    void put(const Point & p);
    bool contains(const Point & p) const;

    std::pair<iterator, iterator> range(const Rect & rect) const;
    iterator begin() const;
    iterator end() const;

    std::optional<Point> nearest(const Point & p) const;
    std::pair<iterator, iterator> nearest(const Point & p, std::size_t k) const;

    friend std::ostream & operator<<(std::ostream & os, const PointSet & tree)
    {
        for (const auto & p : tree) {
            os << p << " ";
        }
        return os;
    }

private:
    using BinaryHeap = std::vector<Distance>;

    void build() const;
    std::shared_ptr<Node> build(std::shared_ptr<Node> & node,
                                std::vector<Point>::iterator first,
                                std::vector<Point>::iterator last,
                                const std::vector<Point>::const_iterator & begin,
                                const Point & min,
                                const Point & max,
                                size_t depth) const;
    void build_if_need() const;
    void preorder_traversal(const std::shared_ptr<Node> & node) const;
    std::shared_ptr<Node> search(const std::shared_ptr<Node> & node, const Point & p) const;
    void range(const std::shared_ptr<Node> & node,
               const Rect & rect,
               const std::shared_ptr<std::vector<std::shared_ptr<Node>>> & vector_pointer) const;
    void nearest(const std::shared_ptr<Node> & node,
                 const Point & point,
                 std::size_t k,
                 BinaryHeap & heap) const;

    mutable std::shared_ptr<Node> root;
    mutable bool need_build;
    std::set<Point> points;
    mutable std::shared_ptr<std::vector<std::shared_ptr<Node>>> dfs;
};

} // namespace kdtree
