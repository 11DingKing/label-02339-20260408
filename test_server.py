#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
环境监测系统 - 测试服务器
用于质检人员测试网络上传功能

使用方法:
    python3 test_server.py [端口号]
    
默认端口: 9000
支持: TCP 和 UDP
"""

import socket
import threading
import sys
import json
from datetime import datetime

# 默认端口
PORT = 9000

# 颜色输出
class Colors:
    GREEN = '\033[92m'
    BLUE = '\033[94m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'
    BOLD = '\033[1m'

def log(msg, color=Colors.END):
    timestamp = datetime.now().strftime('%H:%M:%S')
    print(f"{Colors.BOLD}[{timestamp}]{Colors.END} {color}{msg}{Colors.END}")

def handle_tcp_client(client_socket, address):
    """处理TCP客户端连接"""
    log(f"TCP客户端已连接: {address[0]}:{address[1]}", Colors.GREEN)
    
    try:
        while True:
            data = client_socket.recv(4096)
            if not data:
                break
            
            try:
                # 尝试解析JSON
                decoded = data.decode('utf-8').strip()
                log(f"收到数据 ({len(data)} 字节):", Colors.BLUE)
                
                # 尝试美化JSON输出
                try:
                    json_data = json.loads(decoded)
                    print(json.dumps(json_data, indent=2, ensure_ascii=False))
                except:
                    print(f"  {decoded}")
                    
            except UnicodeDecodeError:
                log(f"收到二进制数据 ({len(data)} 字节): {data.hex()}", Colors.YELLOW)
                
    except ConnectionResetError:
        pass
    except Exception as e:
        log(f"TCP错误: {e}", Colors.RED)
    finally:
        client_socket.close()
        log(f"TCP客户端已断开: {address[0]}:{address[1]}", Colors.YELLOW)

def start_tcp_server(port):
    """启动TCP服务器"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind(('0.0.0.0', port))
        server.listen(5)
        log(f"TCP服务器已启动，监听端口: {port}", Colors.GREEN)
        
        while True:
            client, address = server.accept()
            thread = threading.Thread(target=handle_tcp_client, args=(client, address))
            thread.daemon = True
            thread.start()
            
    except Exception as e:
        log(f"TCP服务器错误: {e}", Colors.RED)
    finally:
        server.close()

def start_udp_server(port):
    """启动UDP服务器"""
    server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind(('0.0.0.0', port))
        log(f"UDP服务器已启动，监听端口: {port}", Colors.GREEN)
        
        while True:
            data, address = server.recvfrom(4096)
            
            try:
                decoded = data.decode('utf-8').strip()
                log(f"UDP收到来自 {address[0]}:{address[1]} 的数据 ({len(data)} 字节):", Colors.BLUE)
                
                try:
                    json_data = json.loads(decoded)
                    print(json.dumps(json_data, indent=2, ensure_ascii=False))
                except:
                    print(f"  {decoded}")
                    
            except UnicodeDecodeError:
                log(f"UDP收到二进制数据: {data.hex()}", Colors.YELLOW)
                
    except Exception as e:
        log(f"UDP服务器错误: {e}", Colors.RED)
    finally:
        server.close()

def main():
    global PORT
    
    if len(sys.argv) > 1:
        try:
            PORT = int(sys.argv[1])
        except ValueError:
            print(f"无效的端口号: {sys.argv[1]}")
            sys.exit(1)
    
    print()
    print(f"{Colors.BOLD}{'='*50}{Colors.END}")
    print(f"{Colors.BOLD}   环境监测系统 - 测试服务器{Colors.END}")
    print(f"{Colors.BOLD}{'='*50}{Colors.END}")
    print()
    print(f"  端口: {PORT}")
    print(f"  支持: TCP / UDP")
    print()
    print(f"  在环境监测系统中配置:")
    print(f"    - 服务器地址: 127.0.0.1")
    print(f"    - 端口: {PORT}")
    print()
    print(f"  按 Ctrl+C 停止服务器")
    print(f"{Colors.BOLD}{'='*50}{Colors.END}")
    print()
    
    # 启动TCP服务器线程
    tcp_thread = threading.Thread(target=start_tcp_server, args=(PORT,))
    tcp_thread.daemon = True
    tcp_thread.start()
    
    # 启动UDP服务器线程
    udp_thread = threading.Thread(target=start_udp_server, args=(PORT,))
    udp_thread.daemon = True
    udp_thread.start()
    
    # 保持主线程运行
    try:
        while True:
            threading.Event().wait(1)
    except KeyboardInterrupt:
        print()
        log("服务器已停止", Colors.YELLOW)

if __name__ == '__main__':
    main()
