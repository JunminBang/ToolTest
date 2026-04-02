"""
언리얼 Remote Execution API를 사용해 unreal_bridge.py를 재시작하는 스크립트.
"""

import sys
import time

REMOTE_EXEC_PATH = r"C:\Program Files\Epic Games\UE_5.7\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
BRIDGE_PATH = r"C:/Unreal/ToolTest/UnrealMCP/unreal_bridge.py"

sys.path.insert(0, REMOTE_EXEC_PATH)
import remote_execution as remote

def restart_bridge():
    re = remote.RemoteExecution()
    re.start()

    # 노드(언리얼 에디터) 검색 대기
    timeout = 5.0
    elapsed = 0.0
    while elapsed < timeout:
        if re.remote_nodes:
            break
        time.sleep(0.2)
        elapsed += 0.2

    nodes = re.remote_nodes
    if not nodes:
        re.stop()
        print("오류: 언리얼 에디터를 찾을 수 없습니다. Remote Execution이 활성화되어 있는지 확인하세요.")
        return False

    node_id = nodes[0]["node_id"]
    re.open_command_connection(node_id)

    cmd = f"exec(open(r'{BRIDGE_PATH}').read())"
    result = re.run_command(cmd, unattended=True)

    re.close_command_connection()
    re.stop()

    if result and result.get("success"):
        print("브릿지 재시작 완료.")
        return True
    else:
        output = result.get("output", "") if result else ""
        print(f"재시작 실패: {output}")
        return False

if __name__ == "__main__":
    restart_bridge()
