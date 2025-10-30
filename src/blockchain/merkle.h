#ifndef DINARI_BLOCKCHAIN_MERKLE_H
#define DINARI_BLOCKCHAIN_MERKLE_H

#include "dinari/types.h"
#include <vector>

namespace dinari {

/**
 * @brief Merkle tree implementation
 *
 * Provides efficient verification of transaction inclusion in blocks.
 * Used for SPV (Simplified Payment Verification) clients.
 */

/**
 * @brief Compute Merkle root from list of hashes
 *
 * @param hashes List of transaction hashes
 * @return Merkle root hash
 */
Hash256 ComputeMerkleRoot(const std::vector<Hash256>& hashes);

/**
 * @brief Merkle branch (proof of inclusion)
 *
 * Contains the hashes needed to verify a transaction is in a block.
 */
class MerkleBranch {
public:
    std::vector<Hash256> branch;  // Hashes in the merkle path
    int index;                    // Index of transaction in block

    MerkleBranch() : index(0) {}

    // Verify that txHash with this branch produces the given merkle root
    bool Verify(const Hash256& txHash, const Hash256& merkleRoot) const;

    // Get size
    size_t Size() const { return branch.size(); }
};

/**
 * @brief Build Merkle branch for transaction at given index
 *
 * @param hashes All transaction hashes in block
 * @param index Index of transaction to build branch for
 * @return Merkle branch
 */
MerkleBranch BuildMerkleBranch(const std::vector<Hash256>& hashes, size_t index);

/**
 * @brief Merkle tree (for efficient branch generation)
 *
 * Stores the complete Merkle tree structure.
 */
class MerkleTree {
public:
    MerkleTree() = default;
    explicit MerkleTree(const std::vector<Hash256>& leaves);

    // Build tree from transaction hashes
    void Build(const std::vector<Hash256>& leaves);

    // Get root hash
    Hash256 GetRoot() const;

    // Get branch for transaction at index
    MerkleBranch GetBranch(size_t index) const;

    // Verify transaction inclusion
    bool VerifyInclusion(const Hash256& txHash, size_t index) const;

    // Get tree depth
    size_t GetDepth() const;

    // Get number of leaves
    size_t GetLeafCount() const { return leafCount; }

private:
    std::vector<std::vector<Hash256>> levels;  // Tree levels (bottom to top)
    size_t leafCount;

    // Compute parent hash from two children
    static Hash256 ComputeParent(const Hash256& left, const Hash256& right);
};

} // namespace dinari

#endif // DINARI_BLOCKCHAIN_MERKLE_H
