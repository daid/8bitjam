#pragma once

#include <vector>
#include <unordered_map>


template<typename T> std::unordered_map<T, float> Dijkstra(T start, std::vector<std::pair<T, float>>(*get_neighbors)(T))
{
    using queueItem = std::pair<int, T>;
    auto cmp = [](queueItem left, queueItem right) { return left.first > right.first; };
    std::priority_queue<queueItem, std::vector<queueItem>, decltype(cmp)> todo{cmp};

    std::unordered_map<T, float> cost_so_far;
    cost_so_far[start] = 0;

    todo.push({0, start});
    while(!todo.empty()) {
        auto [priority, position] = todo.top();
        todo.pop();
        for(auto [pos, c] : get_neighbors(position)) {
            auto new_cost = cost_so_far[position] + c;
            if (cost_so_far.find(pos) == cost_so_far.end() || new_cost < cost_so_far[pos]) {
                cost_so_far[pos] = new_cost;
                priority = new_cost;
                todo.push({priority, pos});
            }
        }
    }
    return cost_so_far;
}