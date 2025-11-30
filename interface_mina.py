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
        self.action = action
        self.font = pygame.font.Font(None, 20)

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
        print(f"DEBUG: Initializing MineManagementSystem...", flush=True)
        print(f"DEBUG: DISPLAY env var: {os.environ.get('DISPLAY')}", flush=True)
        
        pygame.init()
        print("DEBUG: Pygame initialized", flush=True)
        
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
        print("DEBUG: Window created", flush=True)
        
        pygame.display.set_caption("Sistema de Gestão de Mina - ATR")
        self.clock = pygame.time.Clock()
        self.clock = pygame.time.Clock()
        # Use Default Font (Safe for Docker)
        self.font = pygame.font.Font(None, 20)
        self.title_font = pygame.font.Font(None, 24)
        
        # Estado do Sistema
        self.running_simulation = False
        self.processes = []
        # Dictionary of truck states: {id: {x, y, angle, ...}}
        self.truck_states = {} 
        # Initialize for 3 trucks
        for i in range(3):
            self.truck_states[i] = {'x': 0, 'y': 0, 'angle': 0, 'vel': 0, 'temp': 0, 'lidar': 0, 'id': i, 'color': self.get_truck_color(i)}
            
        self.map_data = []
        self.blink_timer = 0
        
        # Planejamento de Rota
        self.planned_routes = {0: [], 1: [], 2: []} # Dict of routes by ID
        self.selected_truck_id = 0
        
        # MQTT
        self.mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "MineManager")
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        self.connected = False
        
        # UI Elements
        self.buttons = [
            Button(820, 20, 160, 40, "INICIAR SIMULAÇÃO", GREEN, self.start_simulation),
            Button(820, 70, 160, 40, "PARAR SIMULAÇÃO", RED, self.stop_simulation),
            # Dropdown-like toggle
            Button(820, 130, 160, 30, "Truck: 0", YELLOW, self.toggle_truck_selection),
            Button(820, 170, 160, 40, "ENVIAR ROTA", BLUE, self.send_route),
            Button(820, 220, 160, 40, "LIMPAR ROTA", GRAY, self.clear_route)
        ]

    def get_truck_color(self, id):
        colors = [YELLOW, (255, 165, 0), (148, 0, 211)] # Yellow, Orange, Violet
        return colors[id % len(colors)]

    def toggle_truck_selection(self):
        self.selected_truck_id = (self.selected_truck_id + 1) % 3
        # Update button text and color
        self.buttons[2].text = f"Truck: {self.selected_truck_id}"
        self.buttons[2].color = self.get_truck_color(self.selected_truck_id)

    def on_connect(self, client, userdata, flags, reason_code, properties):
        print(f"Conectado ao Broker MQTT: {reason_code}")
        self.connected = True
        client.subscribe("caminhao/sensores")
        client.subscribe("caminhao/mapa")
        client.subscribe("caminhao/estado_sistema")
        
    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode()
            data = json.loads(payload)
            if msg.topic == "caminhao/sensores":
                # Update specific truck state
                if 'id' in data:
                    tid = data['id']
                    if tid not in self.truck_states:
                        self.truck_states[tid] = {'x': 0, 'y': 0, 'angle': 0, 'vel': 0, 'temp': 0, 'lidar': 0, 'id': tid, 'color': self.get_truck_color(tid)}
                    
                    # Preserve some local state if needed
                    current_fault = self.truck_states[tid].get('fault', False)
                    current_auto = self.truck_states[tid].get('auto', True)
                    
                    self.truck_states[tid].update(data)
                    
                    if 'fault' not in data: self.truck_states[tid]['fault'] = current_fault
                    if 'auto' not in data: self.truck_states[tid]['auto'] = current_auto
                
            elif msg.topic == "caminhao/mapa":
                if 'map' in data:
                    self.map_data = data['map']
                    print("Mapa recebido!")
            
            elif msg.topic == "caminhao/estado_sistema":
                # Payload: {"manual": bool, "fault": bool}
                # This is global or per truck? Assuming global for now or we need ID.
                # For now, apply to selected truck or all?
                # Let's assume it applies to Truck 0 for backward compatibility or we need to update backend.
                pass 
                
        except Exception as e:
            print(f"Erro MQTT: {e}")

    def start_simulation(self):
        if self.running_simulation:
            print("Simulação já rodando.")
            return

        print("Iniciando simulação...")
        try:
            # 1. Start Mosquitto
            if not self.connected:
                 print("Aguardando conexão com Mosquitto...")
            
            if not self.mqtt_client.is_connected():
                 try:
                    self.mqtt_client.connect(BROKER, PORT, 60)
                    self.mqtt_client.loop_start()
                 except:
                    print("Erro ao conectar MQTT")

            # 3. Start Simulator
            print("Iniciando Simulador (Docker)...")
            p_sim = subprocess.Popen(["docker", "run", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/simulador"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.processes.append(p_sim)
            time.sleep(1)
            
            # 4. Start Controllers (Headless App) - One per truck
            print("Iniciando Controladores (Headless)...")
            for i in range(3):
                cmd_app = ["docker", "run", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/app", str(i)]
                log_file = open(f"app_{i}.log", "w")
                p_app = subprocess.Popen(cmd_app, stdout=log_file, stderr=subprocess.STDOUT)
                self.processes.append(p_app)
            time.sleep(1)

            # 5. Start Cockpit (Interface) - For Truck 0
            print("Iniciando Cockpit (Docker)...")
            cmd_cockpit = ["docker", "run", "-it", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/cockpit"]
            try:
                # Use --wait to keep the terminal open if it crashes immediately
                p_cockpit = subprocess.Popen(["gnome-terminal", "--", "bash", "-c", " ".join(cmd_cockpit)])
            except FileNotFoundError:
                p_cockpit = subprocess.Popen(["xterm", "-e", " ".join(cmd_cockpit)])
            self.processes.append(p_cockpit)

            # 6. Start Simulation Interface (Table)
            print("Iniciando Interface Simulacao (Docker)...")
            cmd_sim_int = ["docker", "run", "-it", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/interface_simulacao"]
            try:
                p_sim_int = subprocess.Popen(["gnome-terminal", "--", "bash", "-c", " ".join(cmd_sim_int)])
            except FileNotFoundError:
                p_sim_int = subprocess.Popen(["xterm", "-e", " ".join(cmd_sim_int)])
            self.processes.append(p_sim_int)
            
            self.running_simulation = True
            
        except Exception as e:
            print(f"Erro ao iniciar processos: {e}")

    def stop_simulation(self):
        if not self.running_simulation:
            return

        print("Parando simulação...")
        for p in self.processes:
            try:
                p.terminate()
                p.wait(timeout=2)
            except:
                try:
                    p.kill()
                except:
                    pass
        self.processes = []
        
        # Force kill any lingering docker containers
        try:
            subprocess.run(["docker", "stop", "atr_cpp_sim", "atr_cpp_app_0", "atr_cpp_app_1", "atr_cpp_app_2", "atr_cpp_cockpit"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            # Also kill by image name to be sure
            subprocess.run(["docker", "ps", "-q", "--filter", "ancestor=atr_cpp"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        except:
            pass

        self.running_simulation = False
        print("Simulação parada.")

        self.connected = False

    def send_route(self):
        if not self.connected:
            print("Não conectado.")
            return
        
        tid = self.selected_truck_id
        route = self.planned_routes[tid]
        
        if not route:
            print(f"Rota vazia para caminhão {tid}.")
            return
            
        route_json = {
            "id": tid,
            "route": [{"x": p[0], "y": p[1], "speed": 20.0} for p in route]
        }
        self.mqtt_client.publish("caminhao/rota", json.dumps(route_json))
        print(f"Rota enviada para caminhão {tid}!")

    def clear_route(self):
        self.planned_routes[self.selected_truck_id] = []

    def handle_map_click(self, pos):
        x, y = pos
        if x < MAP_WIDTH:
            grid_x = (x // CELL_SIZE) * CELL_SIZE + CELL_SIZE/2
            grid_y = (y // CELL_SIZE) * CELL_SIZE + CELL_SIZE/2
            self.planned_routes[self.selected_truck_id].append((grid_x, grid_y))

    def draw_zone(self, x, y, w, h, color, alpha):
        s = pygame.Surface((w, h))
        s.set_alpha(alpha)
        s.fill(color)
        self.screen.blit(s, (x, y))

    def draw(self):
        self.screen.fill(BLACK)
        self.blink_timer += 1
        
        # Draw Map
        if self.map_data:
            for y, row in enumerate(self.map_data):
                for x, cell in enumerate(row):
                    if isinstance(cell, int): cell = chr(cell)
                    rect = pygame.Rect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE)
                    if cell == '1': pygame.draw.rect(self.screen, (100, 50, 50), rect)
                    elif cell == 'A': pygame.draw.rect(self.screen, GREEN, rect)
                    elif cell == 'B': pygame.draw.rect(self.screen, BLUE, rect)
        
        # Draw Grid
        for x in range(0, MAP_WIDTH, CELL_SIZE):
            pygame.draw.line(self.screen, (50, 50, 50), (x, 0), (x, WINDOW_HEIGHT))
        for y in range(0, WINDOW_HEIGHT, CELL_SIZE):
            pygame.draw.line(self.screen, (50, 50, 50), (0, y), (MAP_WIDTH, y))

        # Draw Routes
        for tid, route in self.planned_routes.items():
            color = self.get_truck_color(tid)
            if len(route) > 1:
                pygame.draw.lines(self.screen, color, False, route, 2)
            for p in route:
                pygame.draw.circle(self.screen, color, (int(p[0]), int(p[1])), 4)

        # Draw Trucks
        for tid, state in self.truck_states.items():
            tx = state.get('x', 0)
            ty = state.get('y', 0)
            angle = state.get('angle', 0)
            color = state.get('color', YELLOW)
            
            # Triangle
            rad = math.radians(angle)
            cos_a = math.cos(rad)
            sin_a = math.sin(rad)
            points = [(12, 0), (-8, -8), (-8, 8)]
            rot_points = []
            for px, py in points:
                rx = px * cos_a - py * sin_a
                ry = px * sin_a + py * cos_a
                rot_points.append((tx + rx, ty + ry))
            
            is_fault = state.get('fault', False)
            if is_fault:
                if (self.blink_timer // 10) % 2 == 0: color = RED
            
            pygame.draw.polygon(self.screen, color, rot_points)
            
            # Nametag
            tag_surf = self.font.render(f"ID:{tid}", True, WHITE)
            self.screen.blit(tag_surf, (tx - 15, ty - 25))

        # --- Draw Sidebar (HUD) ---
        pygame.draw.rect(self.screen, (40, 40, 40), (MAP_WIDTH, 0, WINDOW_WIDTH - MAP_WIDTH, WINDOW_HEIGHT))
        pygame.draw.line(self.screen, (100, 100, 100), (MAP_WIDTH, 0), (MAP_WIDTH, WINDOW_HEIGHT), 2)
        
        for btn in self.buttons:
            btn.draw(self.screen)
            
        # Status Info Panel (For Selected Truck)
        info_y = 280
        title = self.title_font.render(f"STATUS (Truck {self.selected_truck_id})", True, WHITE)
        self.screen.blit(title, (MAP_WIDTH + 20, info_y))
        info_y += 30
        
        # Connection
        conn_color = GREEN if self.connected else RED
        conn_text = "CONECTADO" if self.connected else "DESCONECTADO"
        self.screen.blit(self.font.render(f"MQTT: {conn_text}", True, conn_color), (MAP_WIDTH + 20, info_y))
        info_y += 25
        
        # Route Info
        title_route = self.title_font.render("ROTA", True, WHITE)
        self.screen.blit(title_route, (MAP_WIDTH + 20, info_y))
        info_y += 30
        self.screen.blit(self.font.render(f"Waypoints: {len(self.planned_routes[self.selected_truck_id])}", True, LIGHT_GRAY), (MAP_WIDTH + 20, info_y))

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
