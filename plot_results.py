import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import os

def plot_scalability(df, pdf=None):
    subset = df[df['Experiment'] == 'Scalability']
    if subset.empty:
        print("No Scalability data found.")
        return

    fig = plt.figure(figsize=(10, 6))
    plt.plot(subset['Value'], subset['AvgUploadLatency'], marker='o', label='Upload Latency')
    plt.plot(subset['Value'], subset['AvgDownloadLatency'], marker='s', label='Download Latency')
    
    plt.title('Scalability: Latency vs Number of Clients')
    plt.xlabel('Number of Concurrent Clients')
    plt.ylabel('Average Latency (ms)')
    plt.grid(True)
    plt.legend()
    
    if pdf:
        pdf.savefig(fig)
    else:
        plt.savefig('graph_scalability.png')
        print("Generated graph_scalability.png")
    plt.close(fig)

def plot_throughput(df, pdf=None):
    subset = df[df['Experiment'] == 'Throughput']
    if subset.empty:
        print("No Throughput data found.")
        return

    fig = plt.figure(figsize=(10, 6))
    # Convert bytes to MB for better readability
    subset['SizeMB'] = subset['Value'] / (1024 * 1024)
    
    plt.plot(subset['SizeMB'], subset['AvgUploadLatency'], marker='o', label='Upload Latency')
    plt.plot(subset['SizeMB'], subset['AvgDownloadLatency'], marker='s', label='Download Latency')
    
    plt.title('Throughput: Latency vs File Size')
    plt.xlabel('File Size (MB)')
    plt.ylabel('Average Latency (ms)')
    plt.grid(True)
    plt.legend()
    plt.xscale('log') # Log scale for file sizes often looks better
    
    if pdf:
        pdf.savefig(fig)
    else:
        plt.savefig('graph_throughput.png')
        print("Generated graph_throughput.png")
    plt.close(fig)

def plot_data_table(df, pdf):
    fig, ax = plt.subplots(figsize=(12, 8))
    ax.axis('tight')
    ax.axis('off')
    
    # Select columns to display
    cols = ['Experiment', 'Variable', 'Value', 'AvgUploadLatency', 'AvgDownloadLatency', 'SuccessRate']
    table_data = df[cols].values
    
    table = ax.table(cellText=table_data, colLabels=cols, loc='center', cellLoc='center')
    table.auto_set_font_size(False)
    table.set_fontsize(10)
    table.scale(1.2, 1.2)
    
    plt.title('Performance Data Summary', y=0.95)
    
    if pdf:
        pdf.savefig(fig)
    plt.close(fig)

if __name__ == "__main__":
    if not os.path.exists('results.csv'):
        print("Error: results.csv not found!")
        exit(1)

    try:
        df = pd.read_csv('results.csv')
        
        # Generate PNGs
        plot_scalability(df)
        plot_throughput(df)
        
        # Generate PDF Report
        with PdfPages('performance_report.pdf') as pdf:
            # Title Page
            first_page = plt.figure(figsize=(11.69,8.27))
            first_page.clf()
            txt = 'Distributed File Storage\nPerformance Report'
            first_page.text(0.5, 0.5, txt, transform=first_page.transFigure, size=24, ha="center")
            pdf.savefig()
            plt.close()

            plot_scalability(df, pdf)
            plot_throughput(df, pdf)
            plot_data_table(df, pdf)
            
        print("Generated performance_report.pdf")
        
    except ImportError:
        print("Error: pandas or matplotlib not installed. Please run: pip install pandas matplotlib")
    except Exception as e:
        print(f"An error occurred: {e}")
