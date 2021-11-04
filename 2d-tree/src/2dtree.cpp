#include "primitives.h"

#include <fstream>
#include <iostream>

namespace {

Axis get_axis(const size_t depth)
{
    if (depth % 2 == 0) {
        return Axis::X;
    }
    return Axis::Y;
}

Point create_point(const Point & p, const Axis axis, const double coord)
{
    if (axis == Axis::X) {
        return Point(coord, p.y());
    }
    return Point(p.x(), coord);
}

} // anonymous namespace

double Point::distance(const Point & p) const
{
    const double dx = x() - p.x();
    const double dy = y() - p.y();
    return std::sqrt(dx * dx + dy * dy);
}

bool Point::operator==(const Point & p) const
{
    return x() == p.x() && y() == p.y();
}

bool Point::operator!=(const Point & p) const
{
    return !(*this == p);
}

bool Point::operator<(const Point & p) const
{
    return y() < p.y() || (y() == p.y() && x() < p.x());
}

bool Point::operator>(const Point & p) const
{
    return p < *this;
}

bool Point::operator<=(const Point & p) const
{
    return *this < p || *this == p;
}

bool Point::operator>=(const Point & p) const
{
    return !(*this < p);
}

std::string Point::to_string() const
{
    return "(" + std::to_string(x()) + ", " + std::to_string(y()) + ")";
}

std::ostream & operator<<(std::ostream & os, const Point & p)
{
    os << p.to_string();
    return os;
}

double Point::coord(const Axis axis) const
{
    if (axis == Axis::X) {
        return x();
    }
    return y();
}

double Rect::distance(const Point & p) const
{
    double dx = 0.0;
    double dy = 0.0;
    if (p.x() < xmin()) {
        dx = p.x() - xmin();
    }
    else if (p.x() > xmax()) {
        dx = p.x() - xmax();
    }
    if (p.y() < ymin()) {
        dy = p.y() - ymin();
    }
    else if (p.y() > ymax()) {
        dy = p.y() - ymax();
    }
    return std::sqrt(dx * dx + dy * dy);
}

bool Rect::contains(const Point & p) const
{
    return p.x() >= xmin() && p.x() <= xmax() && p.y() >= ymin() && p.y() <= ymax();
}

bool Rect::intersects(const Rect & r) const
{
    return xmax() >= r.xmin() && ymax() >= r.ymin() && r.xmax() >= xmin() && r.ymax() >= ymin();
}

Rect::Rect(const Point & left_bottom_, const Point & right_top_)
    : left_bottom(left_bottom_)
    , right_top(right_top_)
{
}

double Rect::max_coord(const Axis axis) const
{
    return right_top.coord(axis);
}

double Rect::min_coord(const Axis axis) const
{
    return left_bottom.coord(axis);
}

void rbtree::PointSet::put(const Point & p)
{
    if (!contains(p)) {
        set_points.insert(p);
        points->push_back(p);
    }
}

bool rbtree::PointSet::empty() const
{
    return set_points.empty();
}

std::size_t rbtree::PointSet::size() const
{
    return set_points.size();
}

bool rbtree::PointSet::contains(const Point & p) const
{
    return set_points.find(p) != set_points.end();
}

rbtree::PointSet::iterator::iterator(const std::shared_ptr<std::vector<Point>> & vector_pointer_, const std::vector<Point>::iterator & current_)
    : current(current_)
    , vector_pointer(vector_pointer_)
{
}

rbtree::PointSet::iterator::reference rbtree::PointSet::iterator::operator*() const
{
    return *current;
}

rbtree::PointSet::iterator::pointer rbtree::PointSet::iterator::operator->() const
{
    return current.operator->();
}

rbtree::PointSet::iterator & rbtree::PointSet::iterator::operator++()
{
    ++current;
    return *this;
}

rbtree::PointSet::iterator rbtree::PointSet::iterator::operator++(int)
{
    auto tmp = *this;
    ++current;
    return tmp;
}

bool rbtree::PointSet::iterator::operator==(const rbtree::PointSet::iterator & that) const
{
    return current == that.current;
}

bool rbtree::PointSet::iterator::operator!=(const rbtree::PointSet::iterator & that) const
{
    return !(*this == that);
}

bool rbtree::PointSet::iterator::is_valid() const
{
    return vector_pointer != nullptr && current != vector_pointer->end();
}

rbtree::PointSet::iterator rbtree::PointSet::begin() const
{
    return iterator(points, points->begin());
}

rbtree::PointSet::iterator rbtree::PointSet::end() const
{
    return iterator(points, points->end());
}

std::pair<rbtree::PointSet::iterator, rbtree::PointSet::iterator> rbtree::PointSet::range(const Rect & rect) const
{
    const auto range = std::make_shared<std::vector<Point>>();
    for (const auto & p : *this) {
        if (rect.contains(p)) {
            range->push_back(p);
        }
    }
    return std::make_pair(iterator(range, range->begin()), iterator(range, range->end()));
}

std::optional<Point> rbtree::PointSet::nearest(const Point & point) const
{
    if (empty()) {
        return std::nullopt;
    }
    double min_dist = std::numeric_limits<double>::max();
    std::optional<Point> min_point;
    for (const auto & p : *this) {
        const double dist = p.distance(point);
        if (dist < min_dist) {
            min_point.emplace(p);
            min_dist = dist;
        }
    }
    return min_point;
}

std::pair<rbtree::PointSet::iterator, rbtree::PointSet::iterator> rbtree::PointSet::nearest(const Point & point, const std::size_t k) const
{
    if (k >= size()) {
        return std::make_pair(begin(), end());
    }
    if (k == 0) {
        return std::make_pair(end(), end());
    }
    std::vector<double> distances(size());
    std::vector<bool> used(size());
    size_t q = 0;
    for (const auto & p : *this) {
        distances[q] = p.distance(point);
        ++q;
    }
    const auto knearest = std::make_shared<std::vector<Point>>();
    knearest->reserve(k);
    for (size_t i = 0; i < k; ++i) {
        double min = std::numeric_limits<double>::max();
        size_t index_min = std::numeric_limits<size_t>::max();
        auto min_it = end();
        auto it = begin();
        for (size_t j = 0; j < size(); ++j) {
            if (!used[j] && distances[j] < min) {
                min = distances[j];
                index_min = j;
                min_it = it;
            }
            ++it;
        }
        knearest->push_back(*min_it);
        used[index_min] = true;
    }
    return std::make_pair(iterator(knearest, knearest->begin()), iterator(knearest, knearest->end()));
}

rbtree::PointSet::PointSet(const std::string & filename)
    : points(std::make_shared<std::vector<Point>>())
{
    if (!filename.empty()) {
        try {
            std::ifstream fs(filename);

            double x, y;
            while (fs) {
                fs >> x >> y;
                if (fs.fail()) {
                    break;
                }
                put(Point(x, y));
            }
        }
        catch (...) {
            std::cerr << "Can't read " << filename << ".\n";
        }
    }
}

kdtree::PointSet::iterator::iterator(const std::shared_ptr<std::vector<std::shared_ptr<Node>>> & vector_pointer_, const std::vector<std::shared_ptr<Node>>::iterator & current_)
    : current(current_)
    , vector_pointer(vector_pointer_)
{
}

kdtree::PointSet::iterator::reference kdtree::PointSet::iterator::operator*() const
{
    return (*current)->point;
}

kdtree::PointSet::iterator::pointer kdtree::PointSet::iterator::operator->() const
{
    return &(*current)->point;
}

kdtree::PointSet::iterator & kdtree::PointSet::iterator::operator++()
{
    ++current;
    return *this;
}

kdtree::PointSet::iterator kdtree::PointSet::iterator::operator++(int)
{
    auto tmp = *this;
    ++current;
    return tmp;
}

bool kdtree::PointSet::iterator::operator==(const kdtree::PointSet::iterator & that) const
{
    return current == that.current;
}

bool kdtree::PointSet::iterator::operator!=(const kdtree::PointSet::iterator & that) const
{
    return !(*this == that);
}

bool kdtree::PointSet::iterator::is_valid() const
{
    return vector_pointer != nullptr && current != vector_pointer->end();
}

void kdtree::PointSet::put(const Point & p)
{
    need_build = true;
    points.insert(p);
}

void kdtree::PointSet::build() const
{
    std::vector<Point> c;
    std::copy(points.begin(), points.end(), std::back_inserter(c));
    root = build(root,
                 c.begin(),
                 c.end(),
                 c.begin(),
                 Point(std::numeric_limits<double>::min(),
                       std::numeric_limits<double>::min()),
                 Point(std::numeric_limits<double>::max(),
                       std::numeric_limits<double>::max()),
                 0);
    dfs = std::make_shared<std::vector<std::shared_ptr<Node>>>();
    dfs->reserve(size());
    preorder_traversal(root);
}

std::shared_ptr<kdtree::PointSet::Node> kdtree::PointSet::build(std::shared_ptr<Node> & node,
                                                                const std::vector<Point>::iterator first,
                                                                const std::vector<Point>::iterator last,
                                                                const std::vector<Point>::const_iterator & begin,
                                                                const Point & min,
                                                                const Point & max,
                                                                const size_t depth) const
{
    if (first >= last || last == begin) {
        return nullptr;
    }
    const Axis axis = get_axis(depth);
    if (first + 1 == last) {
        return std::make_shared<Node>(*first, Rect(min, max), axis);
    }
    const size_t size = std::distance(first, last);
    const auto middle = first + size / 2;
    std::nth_element(first, middle, last, [&axis](const Point & first, const Point & second) {
        return first.coord(axis) < second.coord(axis);
    });
    if (node == nullptr) {
        node = std::make_shared<Node>(*middle, Rect(min, max), axis);
    }
    const double pivot = middle->coord(axis);
    node->left = build(node->left, first, middle, begin, min, create_point(max, axis, pivot), depth + 1);
    node->right = build(node->right, middle + 1, last, begin, create_point(min, axis, pivot), max, depth + 1);
    return node;
}

void kdtree::PointSet::preorder_traversal(const std::shared_ptr<Node> & node) const
{
    if (node != nullptr) {
        dfs->push_back(node);
        preorder_traversal(node->left);
        preorder_traversal(node->right);
    }
}

bool kdtree::PointSet::contains(const Point & point) const
{
    build_if_need();
    return search(root, point) != nullptr;
}

kdtree::PointSet::iterator kdtree::PointSet::begin() const
{
    build_if_need();
    return iterator(dfs, dfs->begin());
}

kdtree::PointSet::iterator kdtree::PointSet::end() const
{
    build_if_need();
    return iterator(dfs, dfs->end());
}

void kdtree::PointSet::build_if_need() const
{
    if (need_build) {
        need_build = false;
        build();
    }
}

std::shared_ptr<kdtree::PointSet::Node> kdtree::PointSet::search(const std::shared_ptr<Node> & node, const Point & point) const
{
    if (node == nullptr) {
        return nullptr;
    }
    if (*node == point) {
        return node;
    }
    if (*node > point) {
        return search(node->left, point);
    }
    return search(node->right, point);
}

std::pair<kdtree::PointSet::iterator, kdtree::PointSet::iterator> kdtree::PointSet::range(const Rect & r) const
{
    build_if_need();
    auto range_vector = std::make_shared<std::vector<std::shared_ptr<Node>>>();
    range(root, r, range_vector);
    return std::make_pair(iterator(range_vector, range_vector->begin()), iterator(range_vector, range_vector->end()));
}

void kdtree::PointSet::range(const std::shared_ptr<Node> & node, const Rect & rect, const std::shared_ptr<std::vector<std::shared_ptr<Node>>> & r) const
{
    if (node != nullptr && rect.intersects(node->rect)) {
        if (rect.contains(node->point)) {
            r->push_back(node);
        }
        const Axis axis = node->axis;
        const double point_coord = node->point.coord(axis);
        const double min_coord = rect.min_coord(axis);
        const double max_coord = rect.max_coord(axis);
        if (max_coord >= point_coord && min_coord <= point_coord) {
            range(node->left, rect, r);
            range(node->right, rect, r);
        }
        else if (max_coord <= point_coord) {
            range(node->left, rect, r);
        }
        else if (min_coord >= point_coord) {
            range(node->right, rect, r);
        }
    }
}

std::optional<Point> kdtree::PointSet::nearest(const Point & p) const
{
    const auto result = nearest(p, 1);
    if (result.first == result.second) {
        return std::nullopt;
    }
    return *result.first;
}

std::pair<kdtree::PointSet::iterator, kdtree::PointSet::iterator> kdtree::PointSet::nearest(const Point & point, const std::size_t k) const
{
    build_if_need();
    if (empty() || k == 0) {
        return std::make_pair(end(), end());
    }
    if (k >= size()) {
        return std::make_pair(begin(), end());
    }
    BinaryHeap heap;
    std::make_heap(heap.begin(), heap.end());
    heap.reserve(k + 1);
    nearest(root, point, k, heap);
    auto result = std::make_shared<std::vector<std::shared_ptr<Node>>>();
    result->reserve(heap.size());
    for (const auto & d : heap) {
        result->push_back(d.node);
    }
    return std::make_pair(iterator(result, result->begin()), iterator(result, result->end()));
}

void kdtree::PointSet::nearest(const std::shared_ptr<Node> & node, const Point & point, const std::size_t k, BinaryHeap & heap) const
{
    if (node == nullptr) {
        return;
    }
    const double dist = point.distance(node->point);
    if (heap.size() < k || dist < heap[0].distance) {
        heap.emplace_back(dist, node);
        std::push_heap(heap.begin(), heap.end());
        if (heap.size() > k) {
            std::pop_heap(heap.begin(), heap.end());
            heap.pop_back();
        }
    }
    const double coord = point.coord(node->axis);
    const double pivot = node->point.coord(node->axis);
    if (coord < pivot) {
        nearest(node->left, point, k, heap);
        if (coord + heap[0].distance >= pivot) {
            nearest(node->right, point, k, heap);
        }
    }
    else {
        nearest(node->right, point, k, heap);
        if (coord - heap[0].distance <= pivot) {
            nearest(node->left, point, k, heap);
        }
    }
}

kdtree::PointSet::PointSet(const std::string & filename)
    : need_build(true)
{
    if (!filename.empty()) {
        try {
            std::ifstream fs(filename);

            double x, y;
            while (fs) {
                fs >> x >> y;
                if (fs.fail()) {
                    break;
                }
                points.emplace(x, y);
            }

            build_if_need();
        }
        catch (...) {
            std::cout << "Can't read " << filename << ".\n";
        }
    }
}

kdtree::PointSet::Node::Node(const Point & p, const Rect & rect_, const Axis axis_)
    : point(p)
    , rect(rect_)
    , axis(axis_)
{
}

bool kdtree::PointSet::Node::operator>(const Point & p) const
{
    if (axis == Axis::X) {
        if (point.x() == p.x()) {
            return point.y() > p.y();
        }
        return point.x() > p.x();
    }
    if (point.y() == p.y()) {
        return point.x() > p.x();
    }
    return point.y() > p.y();
}

bool kdtree::PointSet::Node::operator<(const Point & p) const
{
    return !(*this > p) && *this != p;
}

bool kdtree::PointSet::Node::operator==(const Point & p) const
{
    return point == p;
}

bool kdtree::PointSet::Node::operator!=(const Point & p) const
{
    return !(*this == p);
}

bool kdtree::PointSet::Distance::operator<(const kdtree::PointSet::Distance & that) const
{
    return distance < that.distance;
}

bool kdtree::PointSet::Distance::operator>(const kdtree::PointSet::Distance & that) const
{
    return distance > that.distance;
}

bool kdtree::PointSet::Distance::operator==(const kdtree::PointSet::Distance & that) const
{
    return distance == that.distance;
}

bool kdtree::PointSet::Distance::operator!=(const kdtree::PointSet::Distance & that) const
{
    return !(*this == that);
}

bool kdtree::PointSet::Distance::operator>=(const kdtree::PointSet::Distance & that) const
{
    return !(*this < that);
}

bool kdtree::PointSet::Distance::operator<=(const kdtree::PointSet::Distance & that) const
{
    return !(*this > that);
}
