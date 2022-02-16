#ifndef libslic3r_SeamPlacerNG_hpp_
#define libslic3r_SeamPlacerNG_hpp_

#include <optional>
#include <vector>
#include <memory>
#include <atomic>

#include "libslic3r/ExtrusionEntity.hpp"
#include "libslic3r/Polygon.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/BoundingBox.hpp"
#include "libslic3r/AABBTreeIndirect.hpp"
#include "libslic3r/KDTreeIndirect.hpp"

namespace Slic3r {

class PrintObject;
class ExtrusionLoop;
class Print;
class Layer;

namespace EdgeGrid {
class Grid;
}

namespace SeamPlacerImpl {

struct GlobalModelInfo;

enum EnforcedBlockedSeamPoint {
    BLOCKED = 0,
    NONE = 1,
    ENFORCED = 2,
};

struct SeamCandidate {
    SeamCandidate(const Vec3f &pos, size_t polygon_index_reverse, float ccw_angle, EnforcedBlockedSeamPoint type) :
            m_position(pos), m_visibility(0.0f), m_overhang(0.0f), m_polygon_index_reverse(polygon_index_reverse), m_seam_index(
                    0), m_ccw_angle(
                    ccw_angle), m_type(type) {
        m_nearby_seam_points = std::make_unique<std::atomic<size_t>>(0);
    }
    Vec3f m_position;
    float m_visibility;
    float m_overhang;
    size_t m_polygon_index_reverse;
    size_t m_seam_index;
    float m_ccw_angle;
    std::unique_ptr<std::atomic<size_t>> m_nearby_seam_points;
    EnforcedBlockedSeamPoint m_type;
};

struct HitInfo {
    Vec3f m_position;
    Vec3f m_surface_normal;
};

struct SeamCandidateCoordinateFunctor {
    SeamCandidateCoordinateFunctor(std::vector<SeamCandidate> *seam_candidates) :
            seam_candidates(seam_candidates) {
    }
    std::vector<SeamCandidate> *seam_candidates;
    float operator()(size_t index, size_t dim) const {
        return seam_candidates->operator[](index).m_position[dim];
    }
};

struct HitInfoCoordinateFunctor {
    HitInfoCoordinateFunctor(std::vector<HitInfo> *hit_points) :
            m_hit_points(hit_points) {
    }
    std::vector<HitInfo> *m_hit_points;
    float operator()(size_t index, size_t dim) const {
        return m_hit_points->operator[](index).m_position[dim];
    }
};
} // namespace SeamPlacerImpl

class SeamPlacer {
public:
    using SeamCandidatesTree =
    KDTreeIndirect<3, float, SeamPlacerImpl::SeamCandidateCoordinateFunctor>;
    static constexpr float expected_hits_per_area = 100.0f;
    static constexpr size_t ray_count = 150000; //NOTE: fixed count of rays is better:
                                                //  on small models, the visibility has huge impact and precision is welcomed.
                                                //  on large models, it would be very expensive to get similar results, and the effect is arguably less important.
    static constexpr float cosine_hemisphere_sampling_power = 1.5f;
    static constexpr float polygon_angles_arm_distance = 0.6f;
    static constexpr float enforcer_blocker_sqr_distance_tolerance = 0.4f;
    static constexpr size_t seam_align_iterations = 4;
    static constexpr size_t seam_align_layer_dist = 30;
    static constexpr float seam_align_tolerable_dist = 0.3f;
    //perimeter points per object per layer idx, and their corresponding KD trees
    std::unordered_map<const PrintObject*, std::vector<std::vector<SeamPlacerImpl::SeamCandidate>>> m_perimeter_points_per_object;
    std::unordered_map<const PrintObject*, std::vector<std::unique_ptr<SeamCandidatesTree>>> m_perimeter_points_trees_per_object;

    void init(const Print &print);

    void place_seam(const Layer* layer, ExtrusionLoop &loop, bool external_first);

private:
    void gather_seam_candidates(const PrintObject* po, const SeamPlacerImpl::GlobalModelInfo& global_model_info);
    void calculate_candidates_visibility(const PrintObject* po, const SeamPlacerImpl::GlobalModelInfo& global_model_info);
    void calculate_overhangs(const PrintObject* po);
    void distribute_seam_positions_for_alignment(const PrintObject* po);
};

} // namespace Slic3r

#endif // libslic3r_SeamPlacerNG_hpp_
