import pygame
import sys
import json
import time
import math
import subprocess
import os
import signal
import paho.mqtt.client as mqtt
import threading

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
        
        # Configurações
        self.NUM_TRUCKS = 3
        
        # Estado do Sistema
        self.running_simulation = False
        self.busy = False # Flag to indicate async operation
        self.processes = []
        # Dictionary of truck states: {id: {x, y, angle, ...}}
        self.truck_states = {} 
        # Initialize for N trucks
        for i in range(self.NUM_TRUCKS):
            self.truck_states[i] = {'x': 0, 'y': 0, 'angle': 0, 'vel': 0, 'temp': 0, 'lidar': 0, 'id': i, 'color': self.get_truck_color(i)}
            
        self.map_data = []
        self.blink_timer = 0
        
        # Planejamento de Rota
        self.planned_routes = {i: [] for i in range(self.NUM_TRUCKS)} # Dict of routes by ID
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
        colors = [YELLOW, (255, 165, 0), (148, 0, 211), (0, 255, 255), (255, 0, 255), (0, 255, 0)] # Expanded palette
        return colors[id % len(colors)]

    def toggle_truck_selection(self):
        self.selected_truck_id = (self.selected_truck_id + 1) % self.NUM_TRUCKS
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
                # Payload: {"id": int, "manual": bool, "fault": bool}
                if 'id' in data:
                    tid = data['id']
                    if tid in self.truck_states:
                        if 'manual' in data:
                            self.truck_states[tid]['auto'] = not data['manual']
                        if 'fault' in data:
                            self.truck_states[tid]['fault'] = data['fault']
                            if data['fault']:
                                print(f"ALERTA: Falha detectada no Caminhão {tid}!", flush=True) 
                
        except Exception as e:
            print(f"Erro MQTT: {e}")

    def on_disconnect(self, client, userdata, rc):
        print(f"Desconectado do Broker MQTT: {rc}")
        self.connected = False

    def start_simulation(self):
        if self.running_simulation or self.busy:
            print("Simulação já rodando ou ocupada.")
            return
        
        self.busy = True
        threading.Thread(target=self._start_simulation_thread, daemon=True).start()

    def _start_simulation_thread(self):
        print("Iniciando simulação (Thread)...")
        try:
            # Reset states
            for i in range(self.NUM_TRUCKS):
                self.truck_states[i] = {'x': 0, 'y': 0, 'angle': 0, 'vel': 0, 'temp': 0, 'lidar': 0, 'id': i, 'color': self.get_truck_color(i)}
            
            # 1. Start Mosquitto
            if not self.mqtt_client.is_connected():
                 print("Conectando ao Mosquitto...")
                 try:
                    self.mqtt_client.connect(BROKER, PORT, 60)
                    self.mqtt_client.loop_start()
                 except Exception as e:
                    print(f"Erro ao conectar MQTT: {e}")
                    self.busy = False
                    return

            # 3. Start Simulator
            print("Iniciando Simulador (Docker)...")
            subprocess.run(["docker", "rm", "-f", "atr_sim"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            # Pass NUM_TRUCKS to simulator if needed (assuming it handles dynamic count or defaults to 3, 
            # but for now we just launch it. Ideally simulator should take an arg)
            p_sim = subprocess.Popen(["docker", "run", "--name", "atr_sim", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/simulador"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.processes.append(p_sim)
            time.sleep(1)
            
            # 4. Start Controllers (Interactive with Embedded Cockpit)
            print("Iniciando Controladores (Embedded Cockpit)...")
            for i in range(self.NUM_TRUCKS):
                container_name = f"atr_app_{i}"
                subprocess.run(["docker", "rm", "-f", container_name], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                
                # Launch in Terminal for Ncurses UI
                cmd_app = ["docker", "run", "--name", container_name, "-it", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/app", str(i)]
                try:
                    p_app = subprocess.Popen(["gnome-terminal", "--title", f"Truck {i} Cockpit", "--", "bash", "-c", " ".join(cmd_app)])
                except FileNotFoundError:
                    p_app = subprocess.Popen(["xterm", "-title", f"Truck {i} Cockpit", "-e", " ".join(cmd_app)])
                
                self.processes.append(p_app)
            time.sleep(1)

            # 5. Start Cockpit (REMOVED - Integrated into App)
            # print("Iniciando Cockpit (Docker)...")
            # subprocess.run(["docker", "rm", "-f", "atr_cockpit"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            # cmd_cockpit = ["docker", "run", "--name", "atr_cockpit", "-it", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/cockpit"]
            # try:
            #     p_cockpit = subprocess.Popen(["gnome-terminal", "--", "bash", "-c", " ".join(cmd_cockpit)])
            # except FileNotFoundError:
            #     p_cockpit = subprocess.Popen(["xterm", "-e", " ".join(cmd_cockpit)])
            # self.processes.append(p_cockpit)

            # 6. Start Interface
            print("Iniciando Interface Simulacao (Docker)...")
            subprocess.run(["docker", "rm", "-f", "atr_interface"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            cmd_sim_int = ["docker", "run", "--name", "atr_interface", "-it", "--rm", "--network=host", "--ipc=host", "atr_cpp", "./bin/interface_simulacao"]
            try:
                p_sim_int = subprocess.Popen(["gnome-terminal", "--", "bash", "-c", " ".join(cmd_sim_int)])
            except FileNotFoundError:
                p_sim_int = subprocess.Popen(["xterm", "-e", " ".join(cmd_sim_int)])
            self.processes.append(p_sim_int)
            
            self.running_simulation = True
            print("Simulação iniciada com sucesso!")
            
        except Exception as e:
            print(f"Erro ao iniciar processos: {e}")
            self.running_simulation = False
        finally:
            self.busy = False

    def stop_simulation(self):
        if not self.running_simulation or self.busy:
            return

        self.busy = True
        threading.Thread(target=self._stop_simulation_thread, daemon=True).start()

    def _stop_simulation_thread(self):
        print("Parando simulação (Thread)...")
        
        # Use Docker RM -F for instant kill
        containers = ["atr_sim", "atr_interface"] + [f"atr_app_{i}" for i in range(self.NUM_TRUCKS)]
        print("Matando containers...", flush=True)
        subprocess.run(["docker", "rm", "-f"] + containers, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
        # Safety Net
        subprocess.run(["docker", "ps", "-q", "--filter", "ancestor=atr_cpp"] + ["|", "xargs", "docker", "rm", "-f"], shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        self.processes = []
        self.running_simulation = False
        self.busy = False
        print("Simulação parada.")

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
            # Update button appearance based on state
            if self.busy:
                btn.color = GRAY
                if btn.text in ["INICIAR SIMULAÇÃO", "PARAR SIMULAÇÃO"]:
                    original_text = btn.text
                    btn.text = "AGUARDE..."
                    btn.draw(self.screen)
                    btn.text = original_text # Restore text for next frame logic
                else:
                    btn.draw(self.screen)
            else:
                # Restore colors
                if btn.text == "INICIAR SIMULAÇÃO": btn.color = GREEN
                elif btn.text == "PARAR SIMULAÇÃO": btn.color = RED
                elif "Truck:" in btn.text: btn.color = self.get_truck_color(self.selected_truck_id)
                elif btn.text == "ENVIAR ROTA": btn.color = BLUE
                elif btn.text == "LIMPAR ROTA": btn.color = GRAY
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
