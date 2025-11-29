import pygame
import sys
import socket
import struct
import json
import time

# Configurações
HOST = '127.0.0.1'
PORT = 65432
CELL_SIZE = 10
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 600

# Cores
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
RED = (255, 0, 0)
GREEN = (0, 255, 0)
BLUE = (0, 0, 255)
GRAY = (128, 128, 128)

class MineSimulation:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("Simulação de Mina - ATR")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont('Arial', 14)
        
        self.map_data = []
        self.truck_state = {
            'x': 0.0, 'y': 0.0, 'angle': 0.0, 'velocity': 0.0,
            'temp': 0.0, 'auto': False, 'fault': False
        }
        
        self.connected = False
        self.socket = None

    def connect(self):
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((HOST, PORT))
            self.connected = True
            print("Conectado ao servidor C++")
        except Exception as e:
            print(f"Erro ao conectar: {e}")
            self.connected = False

    def recv_all(self, n):
        data = b''
        while len(data) < n:
            packet = self.socket.recv(n - len(data))
            if not packet:
                return None
            data += packet
        return data

    def receive_data(self):
        if not self.connected:
            return

        try:
            # Protocolo simples: Tamanho (4 bytes) + JSON
            header = self.recv_all(4)
            if not header:
                return
            
            size = struct.unpack('!I', header)[0]
            data = self.recv_all(size)
            if not data:
                return
            
            state = json.loads(data.decode('utf-8'))
            
            if 'map' in state:
                self.map_data = state['map']
            
            if 'truck' in state:
                self.truck_state = state['truck']
                
        except Exception as e:
            print(f"Erro na recepção: {e}")
            self.connected = False
            self.socket.close()

    def send_command(self, command):
        if not self.connected:
            return
            
        try:
            msg = json.dumps(command).encode('utf-8')
            header = struct.pack('!I', len(msg))
            self.socket.sendall(header + msg)
        except Exception as e:
            print(f"Erro no envio: {e}")

    def draw(self):
        self.screen.fill(BLACK)
        
        # Desenha Mapa
        if self.map_data:
            for y, row in enumerate(self.map_data):
                for x, cell in enumerate(row):
                    rect = pygame.Rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                    if cell == '1':
                        pygame.draw.rect(self.screen, RED, rect)
                    elif cell == '0':
                        pygame.draw.rect(self.screen, GRAY, rect, 1)
        
        # Desenha Caminhão
        tx = self.truck_state['x']
        ty = self.truck_state['y']
        
        # Converte metros para pixels (assumindo escala 1:1 com CELL_SIZE se o C++ mandar em pixels, 
        # mas o C++ manda em metros. Se CELL_SIZE no C++ é 10m, aqui 1px = ? 
        # Vamos assumir que o C++ manda coordenadas compatíveis ou ajustamos aqui.
        # Se C++ manda metros e CELL_SIZE lá é 10.0f.
        # Aqui CELL_SIZE é 10px. Então 1 metro = 1 pixel? Não.
        # Se o grid é 10m, e desenhamos 10px, então 1m = 1px.
        
        # Desenha um triângulo para o caminhão
        # Rotaciona pontos
        import math
        angle_rad = math.radians(self.truck_state['angle'])
        cos_a = math.cos(angle_rad)
        sin_a = math.sin(angle_rad)
        
        # Pontos relativos ao centro (triângulo apontando para direita 0 graus)
        points = [(10, 0), (-5, -5), (-5, 5)]
        rotated_points = []
        for px, py in points:
            rx = px * cos_a - py * sin_a
            ry = px * sin_a + py * cos_a
            rotated_points.append((tx + rx, ty + ry))
            
        color = GREEN if not self.truck_state['fault'] else RED
        pygame.draw.polygon(self.screen, color, rotated_points)
        
        # HUD
        status_text = [
            f"Pos: ({tx:.1f}, {ty:.1f})",
            f"Vel: {self.truck_state['velocity']:.1f} m/s",
            f"Temp: {self.truck_state['temp']:.1f} C",
            f"Modo: {'AUTO' if self.truck_state['auto'] else 'MANUAL'}",
            f"Falha: {'SIM' if self.truck_state['fault'] else 'NAO'}"
        ]
        
        for i, line in enumerate(status_text):
            text_surf = self.font.render(line, True, WHITE)
            self.screen.blit(text_surf, (WINDOW_WIDTH - 180, 10 + i * 20))

        pygame.display.flip()

    def run(self):
        self.connect()
        
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                
                # Comandos de Teclado
                if event.type == pygame.KEYDOWN:
                    cmd = {}
                    if event.key == pygame.K_a: cmd = {'type': 'mode', 'value': 'auto'}
                    elif event.key == pygame.K_m: cmd = {'type': 'mode', 'value': 'manual'}
                    elif event.key == pygame.K_e: cmd = {'type': 'fault', 'kind': 'electric'}
                    elif event.key == pygame.K_h: cmd = {'type': 'fault', 'kind': 'hydraulic'}
                    elif event.key == pygame.K_r: cmd = {'type': 'reset'}
                    
                    if cmd: self.send_command(cmd)

            # Controle Contínuo (Setas)
            keys = pygame.key.get_pressed()
            manual_cmd = {'type': 'control', 'accel': 0, 'steer': 0}
            if keys[pygame.K_UP]: manual_cmd['accel'] = 1
            if keys[pygame.K_RIGHT]: manual_cmd['steer'] = 1
            if keys[pygame.K_LEFT]: manual_cmd['steer'] = -1
            
            if any(manual_cmd.values()):
                self.send_command(manual_cmd)

            self.receive_data()
            self.draw()
            self.clock.tick(30)

        pygame.quit()

if __name__ == "__main__":
    sim = MineSimulation()
    sim.run()
