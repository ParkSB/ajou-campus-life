'''
1. Replace folder name "202312345" with your student !!
   !!WARNING!! you will get 0 score,
   if your folder name is "202312345"
2. Implement Fagin method
3. Implement TA method
4. Implement NRA method

Input: num_dim, top_k
    num_dim: Number of dimension
    top_k: Variable k in top-'k' query
Output: uids_result, cnt_access
    uid_result: Result of top-k uids of the scores.
                The summation function is used
                for the score function.

                i.e., num_dim = 4, k = 2
                ----------------------------
                 uid    D0   D1    D2    D3
                ----------------------------
                "001"    1    1     1     1
                "002"    2    2     2     2
                "003"    3    3     3     3
                "004"    5    5     5     5
                ----------------------------

                score("001") = 1 + 1 + 1 + 1 = 4
                score("002") = 2 + 2 + 2 + 2 = 8
                score("003") = 3 + 3 + 3 + 3 = 12  --> top-2
                score("004") = 4 + 4 + 4 + 4 = 16  --> top-1

                uids_result: ["004", "003"]

    cnt_access: Number of access in each algorithm

Tip: Use Naive method to check your code
     Naive method is the free gift for the code understanding
'''
from collections import defaultdict
from typing import Tuple

def get_score(list_values) -> float:
    result = 0.0
    for v in list_values:
        result += v
    return result

class Algo():
    def __init__(self, list_sorted_entities, uid2dim2value):
        self.list_sorted_entities = list_sorted_entities

        '''
        variable for random access,
        but please do not use this variable directly.
        If you want to get the value of the entity,
        use method 'random_access(uid, dim)'
        '''
        self.__uid2dim2value__ = uid2dim2value

        self.count = len(self.list_sorted_entities[0])

    def random_access(cls, uid, dim) -> float:
        return cls.__uid2dim2value__[uid][dim]

    def find_unknown_dim(cls, uid: str, num_dim: int, uid2dim2value: dict[str, dict[int, float]]):
        result: list[int] = []
        for dim in range(num_dim):
            if dim not in uid2dim2value[uid]:
                result.append(dim)
        return result

    def Naive(cls, num_dim, top_k) -> Tuple[list, int]:
        uids_result = []
        cnt_access = 0

        # read all values from the sorted lists
        uid2dim2value = defaultdict(dict)
        for dim in range(num_dim):
            for uid, value in cls.list_sorted_entities[dim]:
                uid2dim2value[uid][dim] = value
                cnt_access += 1

        # compute the score and sort it
        uid2score = defaultdict(float)
        for uid, dim2value in uid2dim2value.items():
            list_values = []
            for dim in range(num_dim):
                list_values.append(dim2value[dim])
            score = get_score(list_values)
            uid2score[uid] = score

        sorted_uid2score = sorted(uid2score.items(), key = lambda x : -x[1])

        # get the top-k results
        for i in range(top_k):
            uids_result.append(sorted_uid2score[i][0])

        return uids_result, cnt_access

    # Please use random_access(uid, dim) for random access
    def Fagin(cls, num_dim, top_k) -> Tuple[list, int]:
        cnt_access = 0
        cnt_found = 0

        uid2dim2value: defaultdict[str, dict[int, float]] = defaultdict(dict)
        uid2unknown_dim: defaultdict[str, list[int]] = defaultdict(list[int])

        for rank in range(cls.count):
            for dim in range(num_dim):
                uid, value = cls.list_sorted_entities[dim][rank]
                cnt_access += 1

                uid2dim2value[uid][dim] = value

                uid2unknown_dim[uid] = cls.find_unknown_dim(uid, num_dim, uid2dim2value)
                if not len(uid2unknown_dim[uid]):
                    cnt_found += 1
            if cnt_found >= top_k:
                break

        uid2score: defaultdict[str, float] = defaultdict(float)
        for uid in uid2dim2value.keys():
            score = sum(uid2dim2value[uid].values())
            for dim in uid2unknown_dim[uid]:
                score += cls.random_access(uid, dim)
                cnt_access += 1
            uid2score[uid] = score

        uids_result = list(map(lambda x: x[0], sorted(uid2score.items(), key=lambda x: -x[1])))[:top_k]

        return uids_result, cnt_access

    # Please use random_access(uid, dim) for random access
    def TA(cls, num_dim, top_k) -> Tuple[list, int]:
        cnt_access = 0

        uid2dim2value: defaultdict[str, dict[int, float]] = defaultdict(dict)
        top_k_uid2score: list[tuple[str, float]] = []

        for rank in range(cls.count):
            threshold = 0
            uids_to_find: set[str] = set()
            uid2unknown_dim: defaultdict[str, list[int]] = defaultdict(list)

            for dim in range(num_dim):
                uid, value = cls.list_sorted_entities[dim][rank]
                cnt_access += 1

                uid2dim2value[uid][dim] = value
                threshold += value

                uid2unknown_dim[uid] = cls.find_unknown_dim(uid, num_dim, uid2dim2value)
                if len(uid2unknown_dim[uid]):
                    uids_to_find.add(uid)

            uid2score: list[tuple[str, float]] = []
            for uid in uids_to_find:
                score = sum(uid2dim2value[uid].values())
                for dim in uid2unknown_dim[uid]:
                    value = cls.random_access(uid, dim)
                    cnt_access += 1
                    uid2dim2value[uid][dim] = value
                    score += value
                uid2score.append((uid, score))

            top_k_uid2score = sorted(top_k_uid2score + uid2score, key=lambda x: -x[1])[:top_k]
            if len(top_k_uid2score) >= top_k and all(map(lambda x: x[1] >= threshold, top_k_uid2score)):
                break

        uids_result = list(map(lambda x: x[0], top_k_uid2score))
        return uids_result, cnt_access

    def NRA(cls, num_dim, top_k) -> Tuple[list, int]:
        uids_result = []
        cnt_access = 0

        uid2dim2value: defaultdict[str, dict[int, float]] = defaultdict(dict)
        uid2lb: defaultdict[str, float] = defaultdict(float)
        uid2unknown_dims: defaultdict[str, list[int]] = defaultdict(list)

        for rank in range(cls.count):
            curr_rank_dim2value: defaultdict[int, int] = defaultdict(int)
            curr_rank_uids: set[str] = set()

            for dim in range(num_dim):
                uid, value = cls.list_sorted_entities[dim][rank]
                cnt_access += 1

                uid2dim2value[uid][dim] = value
                curr_rank_dim2value[dim] = value
                curr_rank_uids.add(uid)

            for uid in curr_rank_uids:
                lb = round(sum(uid2dim2value[uid].values()), 2)
                uid2lb[uid] = lb
                uid2unknown_dims[uid] = cls.find_unknown_dim(uid, num_dim, uid2dim2value)
                if not len(uid2unknown_dims[uid]):
                    uid2unknown_dims.pop(uid)

            sorted_lbs = sorted(uid2lb.items(), key=lambda x: -x[1])
            uids_result = []
            found = False
            for i in range(len(sorted_lbs)):
                for j in range(i + 1, len(sorted_lbs)):
                    unknown_dims = uid2unknown_dims[sorted_lbs[j][0]]
                    ub = sorted_lbs[j][1]
                    for unknown_dim in unknown_dims:
                        ub += curr_rank_dim2value[unknown_dim]

                    if sorted_lbs[i][1] < ub:
                        found = True
                        break
                if found:
                    break
                uids_result.append(sorted_lbs[i][0])
                if len(uids_result) >= top_k:
                    break

            if len(uids_result) >= top_k:
                break

        return uids_result, cnt_access
