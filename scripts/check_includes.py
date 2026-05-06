#!/usr/bin/env python3
"""
检查头文件包含规则违反
- 分层隔离：Layer N 禁止包含 Layer N+1 或更高
- 循环依赖检测
"""

import os
import re
import sys
from collections import defaultdict

LAYERS = {
    0: "src/domain",
    1: "src/infrastructure",
    2: "src/scheduler",
    3: "src/faulttolerance",
    4: "src/application",
}

def get_layer(file_path):
    """获取文件所在的层"""
    for layer, prefix in LAYERS.items():
        if file_path.startswith(prefix + "/"):
            return layer
    return None

def extract_includes(file_path):
    """提取文件的所有 #include"""
    includes = []
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                match = re.match(r'#include\s+[<"]([^>"]+)[>"]', line)
                if match:
                    includes.append(match.group(1))
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
    return includes

def check_layering(file_path):
    """检查文件是否违反分层规则"""
    violations = []
    source_layer = get_layer(file_path)
    if source_layer is None:
        return violations
    
    includes = extract_includes(file_path)
    for inc in includes:
        # 只检查项目内包含（以 "src/" 开头）
        if inc.startswith('src/'):
            # 正规化路径（移除 ../ 等）
            target_layer = get_layer(inc)
            if target_layer is not None and target_layer > source_layer:
                violations.append({
                    'file': file_path,
                    'include': inc,
                    'source_layer': source_layer,
                    'target_layer': target_layer,
                    'message': f"Layer {source_layer} ({LAYERS[source_layer]}) cannot include Layer {target_layer} ({LAYERS[target_layer]})"
                })
    
    return violations

def check_circular_includes():
    """检查循环依赖"""
    include_graph = defaultdict(set)
    
    # 构建包含图
    for root, dirs, files in os.walk('src'):
        for file in files:
            if file.endswith(('.h', '.cpp')):
                file_path = os.path.join(root, file)
                includes = extract_includes(file_path)
                for inc in includes:
                    if inc.startswith('src/'):
                        # 转换为相对路径
                        target = inc if inc.startswith('src/') else f"src/{inc}"
                        include_graph[file_path].add(target)
    
    # DFS 检测循环
    cycles = []
    visited = set()
    rec_stack = set()
    path_map = {}
    
    def dfs(node, path):
        visited.add(node)
        rec_stack.add(node)
        path_map[node] = path + [node]
        
        for neighbor in include_graph.get(node, []):
            if neighbor not in visited:
                if dfs(neighbor, path + [node]):
                    return True
            elif neighbor in rec_stack:
                # 找到循环
                cycle_start_idx = next((i for i, n in enumerate(path) if n == neighbor), -1)
                if cycle_start_idx >= 0:
                    cycle = path[cycle_start_idx:] + [neighbor]
                    cycles.append(cycle)
                return True
        
        rec_stack.remove(node)
        return False
    
    for node in list(include_graph.keys()):
        if node not in visited:
            dfs(node, [])
    
    return cycles

if __name__ == '__main__':
    violations = []
    
    # 检查分层
    for root, dirs, files in os.walk('src'):
        for file in files:
            if file.endswith(('.h', '.cpp')):
                file_path = os.path.join(root, file)
                violations.extend(check_layering(file_path))
    
    # 检查循环
    cycles = check_circular_includes()
    
    # 输出结果
    if violations or cycles:
        print("❌ Include rule violations found:\n", file=sys.stderr)
        
        if violations:
            print("Layering violations:", file=sys.stderr)
            for v in violations:
                print(f"  {v['file']}:", file=sys.stderr)
                print(f"    {v['message']}", file=sys.stderr)
                print(f"    includes: {v['include']}", file=sys.stderr)
        
        if cycles:
            print("\nCircular dependencies:", file=sys.stderr)
            for cycle in cycles:
                print(f"  {' -> '.join(cycle)}", file=sys.stderr)
        
        sys.exit(1)
    else:
        print("✅ All include rules pass")
        sys.exit(0)
