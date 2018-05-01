#include <utility>

#define JC_VORONOI_IMPLEMENTATION
#include "voronoi.h"

Variant VoronoiEdge::sites() const {
	Vector<Variant> result;
	result.push_back(_diagram->_sites_by_index[Variant(_edge->sites[0]->index)]);
	result.push_back(_diagram->_sites_by_index[Variant(_edge->sites[1]->index)]);
	return result;
}

Vector2 VoronoiEdge::start() const {
	return Vector2(_edge->pos[0].x, _edge->pos[0].y);
}

Vector2 VoronoiEdge::end() const {
	return Vector2(_edge->pos[1].x, _edge->pos[1].y);
}

void VoronoiEdge::_bind_methods() {
	ClassDB::bind_method(D_METHOD("sites"), &VoronoiEdge::sites);
	ClassDB::bind_method(D_METHOD("start"), &VoronoiEdge::start);
	ClassDB::bind_method(D_METHOD("end"), &VoronoiEdge::end);
}

int VoronoiSite::index() const {
	return _site->index;
}

Vector2 VoronoiSite::center() const {
	return Vector2(_site->p.x, _site->p.y);
}

Variant VoronoiSite::edges() const {
	Vector<Variant> result;
	const jcv_graphedge* graphedge = _site->edges;
	while (graphedge) {
		result.push_back(_diagram->_edges_by_address[Variant((int64_t)graphedge->edge)]);
		graphedge = graphedge->next;
	}
	return result;
}

Variant VoronoiSite::neighbors() const {
	Vector<Variant> result;
	const jcv_graphedge* graphedge = _site->edges;
	while (graphedge) {
		if (graphedge->neighbor)
			result.push_back(_diagram->_sites_by_index[Variant(graphedge->neighbor->index)]);
		graphedge = graphedge->next;
	}
	return result;
}

void VoronoiSite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("index"), &VoronoiSite::index);
	ClassDB::bind_method(D_METHOD("center"), &VoronoiSite::center);
	ClassDB::bind_method(D_METHOD("edges"), &VoronoiSite::edges);
	ClassDB::bind_method(D_METHOD("neighbors"), &VoronoiSite::neighbors);
}

VoronoiDiagram::VoronoiDiagram()
	: _diagram(new jcv_diagram) {
	memset(_diagram, 0, sizeof(jcv_diagram));
}

VoronoiDiagram::~VoronoiDiagram() {
	Vector<Variant> gd_edges = _edges;
	for (int i = 0; i < gd_edges.size(); i++)
		memdelete(static_cast<Object*>(gd_edges[i]));

	Vector<Variant> gd_sites = _sites;
	for (int i = 0; i < gd_sites.size(); i++)
		memdelete(static_cast<Object*>(gd_sites[i]));

	jcv_diagram_free(_diagram);
}

Variant VoronoiDiagram::edges() const {
	return _edges;
}

Variant VoronoiDiagram::sites() const {
	return _sites;
}

void VoronoiDiagram::build_objects() {
	Vector<Variant> gd_edges;
	const jcv_edge* edge = jcv_diagram_get_edges(_diagram);
	while (edge) {
		Variant gd_edge = Variant(memnew(VoronoiEdge(edge, this)));
		gd_edges.push_back(gd_edge);
		_edges_by_address[Variant((int64_t)edge)] = gd_edge;
		edge = edge->next;
	}
	_edges = gd_edges;

	Vector<Variant> gd_sites;
	const jcv_site* sites = jcv_diagram_get_sites(_diagram);
	for (int i = 0; i < _diagram->numsites; i++) {
		Variant gd_site = Variant(memnew(VoronoiSite(&sites[i], this)));
		_sites_by_index[Variant(sites[i].index)] = gd_site;
		gd_sites.push_back(gd_site);
	}
	_sites = gd_sites;
}

void VoronoiDiagram::_bind_methods() {
	ClassDB::bind_method(D_METHOD("edges"), &VoronoiDiagram::edges);
	ClassDB::bind_method(D_METHOD("sites"), &VoronoiDiagram::sites);
}

void Voronoi::set_points(Vector<Vector2> points) {
	assert(points.size());

	std::vector<jcv_point> new_points;
	new_points.reserve(points.size());

	// translate Godot Vector2 points into jcv_points
	for (int i = 0; i < points.size(); i++) {
		jcv_point point = { points[i].x, points[i].y };
		new_points.push_back(point);
	}

	_points.swap(new_points);
}

void Voronoi::set_boundaries(Rect2 boundaries) {
	jcv_point p1 = { boundaries.position.x, boundaries.position.y };
	jcv_point p2 = { boundaries.position.x + boundaries.size.x, boundaries.position.y + boundaries.size.y };
	jcv_rect * rect = new jcv_rect();
	rect->min = p1;
	rect->max = p2;
	_boundaries.reset(rect);
}

void* useralloc(void* ctx, size_t size) {
	return memalloc(size);
}

void userfree(void* ctx, void* ptr) {
	return memfree(ptr);
}

Ref<VoronoiDiagram> Voronoi::generate_diagram() const {
	Ref<VoronoiDiagram> result = memnew(VoronoiDiagram);
	jcv_diagram_generate_useralloc(_points.size(), _points.data(), _boundaries.get(), NULL, &useralloc, &userfree, result->_diagram);
	result->build_objects();
	return result;
}

void Voronoi::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_points", "points"), &Voronoi::set_points);
	ClassDB::bind_method(D_METHOD("set_boundaries", "boundaries"), &Voronoi::set_boundaries);
	ClassDB::bind_method(D_METHOD("generate_diagram"), &Voronoi::generate_diagram);
}
