from typing import Optional


class TNode:

    def __init__(self, data):
        self.data = data
        self.left = None
        self.right = None


class Solution:

    def compare(self, a: Optional[TNode], b: Optional[TNode]) -> bool:

        if a is None and b is None:
            return True
        if a is None or b is None:
            return False
        return a.data == b.data and self.compare(a.left, b.left) and self.compare(a.right, b.right)


if __name__ == "__main__":

    # Test 1
    # check to see if two, two level trees are equal: expected True
    root1 = TNode(1)
    root1.left = TNode(2)
    root1.right = TNode(3)
    root2 = TNode(1)
    root2.left = TNode(2)
    root2.right = TNode(3)
    print(Solution().compare(root1, root2))

    # Test 2
    # check to see if two, two level trees are not equal: expected False
    root1 = TNode(1)
    root1.left = TNode(2)
    root1.right = TNode(3)
    root2 = TNode(1)
    root2.left = TNode(2)
    root2.right = TNode(4)
    print(Solution().compare(root1, root2))
