import matplotlib.pyplot as plt
import pandas as pd
try:
    data = pd.read_csv('sensor_data.csv')
    plt.figure(figsize=(10, 6))
    plt.plot(data['Index'], data['RawX'], label='Raw X', alpha=0.5)
    plt.plot(data['Index'], data['FilteredX'], label='Filtered X (EMA)', linewidth=2)
    plt.title('Sensor X: Raw vs Filtered (Exponential Moving Average)')
    plt.xlabel('Sample Index')
    plt.ylabel('Position X')
    plt.legend()
    plt.grid(True)
    plt.savefig('sensor_plot.png')
    print('Gráfico salvo como sensor_plot.png')
except Exception as e:
    print(f'Erro ao gerar gráfico: {e}')
