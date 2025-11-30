from sys import argv
import os
import re
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

def collect(n, m, out_path):
    outputs = os.path.join('..', 'example', 'output')

    with open(out_path, "w") as outfile:
        outfile.writelines([
            "pid,round,cpu,wc,ram,seq_nr,log_r,urb_s,urb_r,beb_s,beb_r,pf_s,pf_r,sent,s_cyc,ack_s,ack_r,ack_cyc,recv,recvtot,aas,aar,lookup_size,acked_size,p_acks_size,fl_s,fl_r,fl_rq,msgs_in_q\n"
        ])

        for in_path in os.listdir(outputs):
            if not re.match('proc\\d+.stdout', in_path):
                continue
            if int(in_path.split('.')[0][4:]) > n:
                continue
            with open(os.path.join(outputs, in_path)) as infile:
                outfile.writelines(infile.readlines())

if __name__ == "__main__":
    n, m, c = int(argv[1]), int(argv[2]), bool(int(argv[3]))

    out_path = os.path.join('..', 'example', 'stats', f'{n}-{m}.csv')
    if c:
        collect(n, m, out_path)
    out_dir = os.path.dirname(out_path)
    df = pd.read_csv(out_path)

    # cutoff = df[(~df['is_r']) & (df['m_cmpsd'] == 0)]['round'].min()
    # print(cutoff)
    # df.drop(index=df[df['round'] >= cutoff].index, inplace=True)

    df['aa_r'] = df['aas'] / df['aar']
    df['ack_r'] = df['ack_s'] / df['ack_r']
    df['fl_r'] = df['fl_s'] / df['fl_r']
    df['fl_rq'] = df['fl_s'] / df['fl_rq']

    sns.lineplot(data=df, x='round', y='fl_r', label='FL SR ratio', palette=sns.color_palette('Set2'))
    sns.lineplot(data=df, x='round', y='fl_rq', label='FL SR ratio Q', palette=sns.color_palette('Set2'))
    sns.lineplot(data=df, x='round', y='ack_r', label='Ack SR ratio', palette=sns.color_palette('Set2'))
    sns.lineplot(data=df, x='round', y='aa_r', label='AckAck SR ratio', palette=sns.color_palette('Set2'))
    flf = df[df['round'] <= df['round'].max()/2]['fl_r'].mean()
    flfq = df[df['round'] <= df['round'].max()/2]['fl_rq'].mean()
    a = df[df['round'] <= df['round'].max()/2]['ack_r'].mean()
    aa = df[df['round'] <= df['round'].max()/2]['aa_r'].mean()
    plt.title(f'FLF {flf:02.2f}, FLFQ {flfq:02.2f}, Ack {a:02.2f}, AckAck {aa:02.2f}')
    plt.savefig(os.path.join(out_dir, f'{n}-{m}-ratios.png'))
    plt.clf()
    
    df['send_r'] = df['pf_s'] / df['sent']
    df['recv_r'] = df['pf_r'] / df['recv']
    sns.lineplot(data=df, x='round', y='send_r', label='Perf / Stub Send Ratio')
    sns.lineplot(data=df, x='round', y='recv_r', label='Perf / Stub Recv Ratio (dupes)')
    s_r_mean = df[df['round'] <= df['round'].max()/2]['send_r'].mean()
    r_r_mean = df[df['round'] <= df['round'].max()/2]['recv_r'].mean()
    plt.title(f'S Mean: {s_r_mean:02.2f}, R Mean: {r_r_mean:02.2f}')
    plt.savefig(os.path.join(out_dir, f'{n}-{m}-PS.png'))
    plt.clf()

    sns.lineplot(data=df, x='wc', hue='pid', y='cpu')
    plt.savefig(os.path.join(out_dir, f'{n}-{m}-CPU.png'))
    plt.clf()
    
    sns.lineplot(data=df, x='wc', hue='pid', y='ram')
    plt.savefig(os.path.join(out_dir, f'{n}-{m}-RAM.png'))
    plt.clf()

    r_log_mean = df[df['round'] <= df['round'].max()/2]['log_r'].mean()
    s_log_mean = df[df['round'] <= df['round'].max()/2]['seq_nr'].mean()
    sns.lineplot(data=df, x='round', y='log_r')
    sns.lineplot(data=df, x='round', y='seq_nr')
    plt.title(f'R Mean: {r_log_mean:.0f}, S Mean: {s_log_mean:.0f}')
    plt.savefig(os.path.join(out_dir, f'{n}-{m}-LOGS.png'))


