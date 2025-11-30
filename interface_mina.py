import pygame
import sys
import json
import time
import math
import subprocess
import os
import signal
import paho.mqtt.client as mqtt

# Configurações
BROKER = '127.0.0.1'
PORT = 1883
CELL_SIZE = 10
WINDOW_WIDTH = 1000  # Aumentado para sidebar
WINDOW_HEIGHT = 600
MAP_WIDTH = 800      # Largura do mapa

# Cores
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
RED = (231, 76, 60)      # Alizarin (Muted Red)
GREEN = (46, 204, 113)   # Emerald (Muted Green)
BLUE = (52, 152, 219)    # Peter River (Muted Blue)
GRAY = (149, 165, 166)   # Concrete (Muted Gray)
DARK_GRAY = (50, 50, 50)
LIGHT_GRAY = (236, 240, 241) # Clouds
YELLOW = (241, 196, 15)  # Sunflower

class Button:
    def __init__(self, x, y, w, h, text, color, action):
        self.rect = pygame.Rect(x, y, w, h)
        self.text = text
        self.color = color
        self.action = action
        self.font = pygame.font.SysFont('Arial', 14, bold=True)

    def draw(self, screen):
        pygame.draw.rect(screen, self.color, self.rect)
        pygame.draw.rect(screen, WHITE, self.rect, 2)
        text_surf = self.font.render(self.text, True, BLACK)
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if self.rect.collidepoint(event.pos):
                self.action()

class MineManagementSystem:
    def __init__(self):
        pygame.init()
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        pygame.display.set_caption("Sistema de Gestão de Mina - ATR")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont('Arial', 14)
        
        # Estado do Sistema
        self.running_simulation = False
        self.processes = []
        self.truck_state = {'x': 0, 'y': 0, 'angle': 0, 'vel': 0, 'temp': 0, 'lidar': 0}
        self.map_data = []
        
        # Planejamento de Rota
        self.planned_route = [] # Lista de (x, y)
        
        # MQTT
        self.mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "MineManager")
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        self.connected = False
        
        # UI Elements
        self.buttons = [
            Button(820, 20, 160, 40, "INICIAR SIMULAÇÃO", GREEN, self.start_simulation),
            Button(820, 70, 160, 40, "PARAR SIMULAÇÃO", RED, self.stop_simulation),
            Button(820, 150, 160, 40, "ENVIAR ROTA", BLUE, self.send_route),
            Button(820, 200, 160, 40, "LIMPAR ROTA", GRAY, self.clear_route)
        ]

    def on_connect(self, client, userdata, flags, reason_code, properties):
        print(f"Conectado ao Broker MQTT: {reason_code}")
        self.connected = True
        client.subscribe("caminhao/sensores")
        client.subscribe("caminhao/mapa")
        
    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode()
            data = json.loads(payload)
            if msg.topic == "caminhao/sensores":
                self.truck_state = data
            elif msg.topic == "caminhao/mapa":
                if 'map' in data:
                    self.map_data = data['map']
                    print("Mapa recebido!")
        except Exception as e:
            print(f"Erro MQTT: {e}")

    def start_simulation(self):
        if self.running_simulation:
            print("Simulação já rodando.")
            return

        print("Iniciando simulação...")
        try:
            # 1. Start Mosquitto
            p_mosq = subprocess.Popen(["mosquitto", "-c", "mosquitto.conf", "-d"])
            self.processes.append(p_mosq)
            time.sleep(1)
            
            # 2. Connect MQTT Client
            try:
                self.mqtt_client.connect(BROKER, PORT, 60)
                self.mqtt_client.loop_start()
            except:
                print("Aguardando broker...")
            
            # 3. Start Simulator
            p_sim = subprocess.Popen(["./bin/simulador"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.processes.append(p_sim)
            time.sleep(1)
            
            # 4. Start Controller
            p_app = subprocess.Popen(["./bin/app"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.processes.append(p_app)
            
            self.running_simulation = True
            
        except Exception as e:
            print(f"Erro ao iniciar processos: {e}")

    def stop_simulation(self):
        print("Parando simulação...")
        for p in self.processes:
            p.terminate()
        
        # Kill force just in case
        os.system("pkill -f bin/simulador")
        os.system("pkill -f bin/app")
        os.system("pkill -f mosquitto")
        
        self.processes = []
        self.running_simulation = False
        self.mqtt_client.loop_stop()
        self.connected = False

    def send_route(self):
        if not self.connected or not self.planned_route:
            print("Não conectado ou rota vazia.")
            return
            
        route_json = {
            "route": [{"x": p[0], "y": p[1], "speed": 20.0} for p in self.planned_route]
        }
        self.mqtt_client.publish("caminhao/rota", json.dumps(route_json))
        print("Rota enviada!")

    def clear_route(self):
        self.planned_route = []

    def handle_map_click(self, pos):
        x, y = pos
        if x < MAP_WIDTH:
            # Convert pixel to meter (assuming 10px = 10m for simplicity in visualization, 
            # but simulator logic uses meters directly. Let's assume 1px = 1m for simplicity 
            # or keep the grid logic.
            # In simulator: 61x61 grid, CELL_SIZE=10. So 610x610 meters.
            # Window is 800x600.
            # Let's map click directly to meters.
            
            # Snap to grid (10m)
            grid_x = (x // CELL_SIZE) * CELL_SIZE + CELL_SIZE/2
            grid_y = (y // CELL_SIZE) * CELL_SIZE + CELL_SIZE/2
            
            self.planned_route.append((grid_x, grid_y))

    def draw(self):
        self.screen.fill(BLACK)
        
        # --- Draw Map Area ---
        # pygame.draw.rect(self.screen, (20, 20, 20), (0, 0, MAP_WIDTH, WINDOW_HEIGHT))
        
        # Draw Map from Data
        if self.map_data:
            for y, row in enumerate(self.map_data):
                for x, cell in enumerate(row):
                    # Handle ASCII codes sent by C++ json serializer
                    if isinstance(cell, int):
                        cell = chr(cell)
                        
                    rect = pygame.Rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                    if cell == '1': # Wall
                        pygame.draw.rect(self.screen, (100, 50, 50), rect)
                    elif cell == '0': # Path
                        pygame.draw.rect(self.screen, (200, 200, 200), rect)
                    elif cell == 'A': # Start
                        pygame.draw.rect(self.screen, GREEN, rect)
                    elif cell == 'B': # End
                        pygame.draw.rect(self.screen, RED, rect)
        else:
            # Draw Grid if no map
            for x in range(0, MAP_WIDTH, CELL_SIZE):
                pygame.draw.line(self.screen, (30, 30, 30), (x, 0), (x, WINDOW_HEIGHT))
            for y in range(0, WINDOW_HEIGHT, CELL_SIZE):
                pygame.draw.line(self.screen, (30, 30, 30), (0, y), (MAP_WIDTH, y))

        # Draw Planned Route
        if len(self.planned_route) > 1:
            pygame.draw.lines(self.screen, BLUE, False, self.planned_route, 2)
        for p in self.planned_route:
            pygame.draw.circle(self.screen, BLUE, (int(p[0]), int(p[1])), 4)

        # Draw Truck
        tx = self.truck_state.get('x', 0)
        ty = self.truck_state.get('y', 0)
        angle = self.truck_state.get('angle', 0)
        
        # Triangle
        rad = math.radians(angle)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        points = [(15, 0), (-10, -10), (-10, 10)]
        rot_points = []
        for px, py in points:
            rx = px * cos_a - py * sin_a
            ry = px * sin_a + py * cos_a
            rot_points.append((tx + rx, ty + ry))
        
        color = GREEN
        if self.truck_state.get('fault', False): color = RED
        elif not self.truck_state.get('auto', True): color = YELLOW # Manual
        
        pygame.draw.polygon(self.screen, color, rot_points)

        # --- Draw Sidebar ---
        pygame.draw.rect(self.screen, LIGHT_GRAY, (MAP_WIDTH, 0, WINDOW_WIDTH - MAP_WIDTH, WINDOW_HEIGHT))
        
        for btn in self.buttons:
            btn.draw(self.screen)
            
        # Status Info
        info_y = 300
        lines = [
            f"Status: {'RODANDO' if self.running_simulation else 'PARADO'}",
            f"MQTT: {'CONECTADO' if self.connected else 'DESCONECTADO'}",
            "",
            "--- CAMINHÃO ---",
            f"Pos: ({tx:.1f}, {ty:.1f})",
            f"Vel: {self.truck_state.get('vel', 0):.1f} m/s",
            f"Temp: {self.truck_state.get('temp', 0)} C",
            f"Lidar: {self.truck_state.get('lidar', 0)} m",
            "",
            "--- ROTA ---",
            f"Waypoints: {len(self.planned_route)}"
        ]
        
        for line in lines:
            surf = self.font.render(line, True, BLACK)
            self.screen.blit(surf, (MAP_WIDTH + 10, info_y))
            info_y += 20

        pygame.display.flip()

    def run(self):
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.stop_simulation()
                    running = False
                
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if event.pos[0] < MAP_WIDTH:
                        self.handle_map_click(event.pos)
                
                for btn in self.buttons:
                    btn.handle_event(event)
            
            self.draw()
            self.clock.tick(30)
        
        pygame.quit()

if __name__ == "__main__":
    app = MineManagementSystem()
    app.run()
