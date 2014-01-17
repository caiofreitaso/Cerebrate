#include "Util.h"

Cerebrate::TerrainAnalysis::Polygon Cerebrate::TerrainAnalysis::convert(BWTA::Polygon p) {
	Cerebrate::TerrainAnalysis::Polygon ret;
	unsigned i = 0;
	for (; i < p.size() -1; i++)
		ret.push_back(Cerebrate::TerrainAnalysis::Segment(Cerebrate::TerrainAnalysis::Point(p[i]),Cerebrate::TerrainAnalysis::Point(p[i+1])));
	ret.push_back(Cerebrate::TerrainAnalysis::Segment(Cerebrate::TerrainAnalysis::Point(p[i]),Cerebrate::TerrainAnalysis::Point(p[0])));
	
	return ret;
}		
bool Cerebrate::TerrainAnalysis::in(Cerebrate::TerrainAnalysis::Polygon& p, Cerebrate::TerrainAnalysis::Point a) {
	int x = 0;
	
	Cerebrate::TerrainAnalysis::Ray r(a);
	
	for (unsigned i = 0; i < p.size(); i++) {
		if (r.intersect(p[i]))
			x++;
	}
	
	return x % 2 == 1;
}
bool Cerebrate::TerrainAnalysis::in(Cerebrate::TerrainAnalysis::Polygon& p, BWAPI::Position a) {
	return in(p,Point(a));
}
bool Cerebrate::TerrainAnalysis::in(Cerebrate::TerrainAnalysis::Polygon& p, BWAPI::TilePosition a) {
	return in(p,Point(a));
}


Cerebrate::TerrainAnalysis::Point operator-(Cerebrate::TerrainAnalysis::Point a, Cerebrate::TerrainAnalysis::Point b) {
	return Cerebrate::TerrainAnalysis::Point(a.x - b.x, a.y - b.y);
}
Cerebrate::TerrainAnalysis::Point operator+(Cerebrate::TerrainAnalysis::Point a, Cerebrate::TerrainAnalysis::Point b){
	return Cerebrate::TerrainAnalysis::Point(a.x + b.x, a.y + b.y);
}
Cerebrate::TerrainAnalysis::Point operator*(double a, Cerebrate::TerrainAnalysis::Point b) {
	return Cerebrate::TerrainAnalysis::Point(a*b.x, a*b.y);
}
Cerebrate::TerrainAnalysis::Point operator*(Cerebrate::TerrainAnalysis::Point a, double b) {
	return Cerebrate::TerrainAnalysis::Point(a.x*b, a.y*b);
}
Cerebrate::TerrainAnalysis::Point operator/(Cerebrate::TerrainAnalysis::Point a, double b) {
	return Cerebrate::TerrainAnalysis::Point(a.x/b, a.y/b);
}
Cerebrate::TerrainAnalysis::Point operator~(Cerebrate::TerrainAnalysis::Point a) {
	return a/a.length();
}