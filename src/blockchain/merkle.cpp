#include "merkle.h"
#include "crypto/hash.h"

namespace dinari {

Hash256 ComputeMerkleRoot(const std::vector<Hash256>& hashes) {
    return crypto::Hash::ComputeMerkleRoot(hashes);
}

// MerkleBranch implementation

bool MerkleBranch::Verify(const Hash256& txHash, const Hash256& merkleRoot) const {
    Hash256 hash = txHash;
    int idx = index;

    for (const auto& branchHash : branch) {
        bytes combined;
        combined.reserve(64);

        // Determine order based on index
        if (idx & 1) {
            // Odd index: branchHash is on the left
            combined.insert(combined.end(), branchHash.begin(), branchHash.end());
            combined.insert(combined.end(), hash.begin(), hash.end());
        } else {
            // Even index: branchHash is on the right
            combined.insert(combined.end(), hash.begin(), hash.end());
            combined.insert(combined.end(), branchHash.begin(), branchHash.end());
        }

        hash = crypto::Hash::DoubleSHA256(combined);
        idx >>= 1;  // Move to parent level
    }

    return hash == merkleRoot;
}

MerkleBranch BuildMerkleBranch(const std::vector<Hash256>& hashes, size_t index) {
    MerkleBranch branch;
    branch.index = static_cast<int>(index);

    if (hashes.empty() || index >= hashes.size()) {
        return branch;
    }

    std::vector<Hash256> level = hashes;
    size_t idx = index;

    while (level.size() > 1) {
        // Determine sibling index
        size_t siblingIdx;
        if (idx & 1) {
            // Odd index: sibling is on the left
            siblingIdx = idx - 1;
        } else {
            // Even index: sibling is on the right
            siblingIdx = idx + 1;
        }

        // Add sibling to branch
        if (siblingIdx < level.size()) {
            branch.branch.push_back(level[siblingIdx]);
        } else {
            // If no sibling (odd number of nodes), duplicate the last one
            branch.branch.push_back(level[idx]);
        }

        // Build next level
        std::vector<Hash256> nextLevel;
        for (size_t i = 0; i < level.size(); i += 2) {
            bytes combined;
            combined.reserve(64);
            combined.insert(combined.end(), level[i].begin(), level[i].end());

            if (i + 1 < level.size()) {
                combined.insert(combined.end(), level[i + 1].begin(), level[i + 1].end());
            } else {
                // Duplicate if odd number
                combined.insert(combined.end(), level[i].begin(), level[i].end());
            }

            nextLevel.push_back(crypto::Hash::DoubleSHA256(combined));
        }

        level = std::move(nextLevel);
        idx >>= 1;  // Move to parent level
    }

    return branch;
}

// MerkleTree implementation

MerkleTree::MerkleTree(const std::vector<Hash256>& leaves)
    : leafCount(0) {
    Build(leaves);
}

void MerkleTree::Build(const std::vector<Hash256>& leaves) {
    levels.clear();
    leafCount = leaves.size();

    if (leaves.empty()) {
        return;
    }

    // Add leaf level
    levels.push_back(leaves);

    // Build tree levels bottom-up
    while (levels.back().size() > 1) {
        const auto& currentLevel = levels.back();
        std::vector<Hash256> nextLevel;

        for (size_t i = 0; i < currentLevel.size(); i += 2) {
            if (i + 1 < currentLevel.size()) {
                nextLevel.push_back(ComputeParent(currentLevel[i], currentLevel[i + 1]));
            } else {
                // Odd number of nodes, duplicate the last one
                nextLevel.push_back(ComputeParent(currentLevel[i], currentLevel[i]));
            }
        }

        levels.push_back(std::move(nextLevel));
    }
}

Hash256 MerkleTree::GetRoot() const {
    if (levels.empty() || levels.back().empty()) {
        return Hash256{};
    }
    return levels.back()[0];
}

MerkleBranch MerkleTree::GetBranch(size_t index) const {
    MerkleBranch branch;
    branch.index = static_cast<int>(index);

    if (levels.empty() || index >= levels[0].size()) {
        return branch;
    }

    size_t idx = index;

    // Traverse from leaf to root, collecting sibling hashes
    for (size_t level = 0; level < levels.size() - 1; ++level) {
        size_t siblingIdx = (idx & 1) ? idx - 1 : idx + 1;

        if (siblingIdx < levels[level].size()) {
            branch.branch.push_back(levels[level][siblingIdx]);
        } else {
            // No sibling (odd number), use current node
            branch.branch.push_back(levels[level][idx]);
        }

        idx >>= 1;  // Move to parent level
    }

    return branch;
}

bool MerkleTree::VerifyInclusion(const Hash256& txHash, size_t index) const {
    if (levels.empty() || index >= levels[0].size()) {
        return false;
    }

    // Check if transaction hash matches at leaf level
    if (levels[0][index] != txHash) {
        return false;
    }

    // Verify merkle path
    MerkleBranch branch = GetBranch(index);
    return branch.Verify(txHash, GetRoot());
}

size_t MerkleTree::GetDepth() const {
    return levels.size();
}

Hash256 MerkleTree::ComputeParent(const Hash256& left, const Hash256& right) {
    bytes combined;
    combined.reserve(64);
    combined.insert(combined.end(), left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    return crypto::Hash::DoubleSHA256(combined);
}

} // namespace dinari
