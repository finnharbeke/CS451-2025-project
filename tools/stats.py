from sys import argv
import os
import re
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

def collect(p, out_path):
    outputs = os.path.join('..', 'example', 'output')

    with open(out_path, "w") as outfile:
        outfile.writelines([
            "pid,round,cpu,wc,ram,logged,decided,agr_rounds,restarts,loaded_tail,active_tail,active_front,loaded_front,beb_s,beb_r,pf_s,pf_r,sent,s_cyc,ack_s,ack_r,ack_cyc,recv,recvtot,aas,aar,lookup_size,acked_size,p_acks_size,fl_s,fl_r,fl_rq,msgs_in_q\n"
        ])

        for in_path in os.listdir(outputs):
            if not re.match('proc\\d+.stdout', in_path):
                continue
            if int(in_path.split('.')[0][4:]) > p:
                continue
            with open(os.path.join(outputs, in_path)) as infile:
                outfile.writelines(infile.readlines())

if __name__ == "__main__":
    p, n, vs, ds, c = int(argv[1]), int(argv[2]), int(argv[3]), int(argv[4]), bool(int(argv[5]))

    out_path = os.path.join('..', 'example', 'stats', f'{p}-{n}-{vs}-{ds}.csv')
    if c:
        collect(p, out_path)
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
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-ratios.png'))
    plt.clf()
    
    df['send_r'] = df['pf_s'] / df['sent']
    df['recv_r'] = df['pf_r'] / df['recv']
    sns.lineplot(data=df, x='round', y='send_r', label='Perf / Stub Send Ratio')
    sns.lineplot(data=df, x='round', y='recv_r', label='Perf / Stub Recv Ratio (dupes)')
    s_r_mean = df[df['round'] <= df['round'].max()/2]['send_r'].mean()
    r_r_mean = df[df['round'] <= df['round'].max()/2]['recv_r'].mean()
    plt.title(f'S Mean: {s_r_mean:02.2f}, R Mean: {r_r_mean:02.2f}')
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-PS.png'))
    plt.clf()

    sns.lineplot(data=df, x='wc', hue='pid', y='cpu')
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-CPU.png'))
    plt.clf()
    
    sns.lineplot(data=df, x='wc', hue='pid', y='ram')
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-RAM.png'))
    plt.clf()

    logged_mean = df[df['round'] <= df['round'].max()/2]['logged'].mean()
    sns.lineplot(data=df, x='round', y='logged')
    plt.title(f'Mean: {logged_mean:.0f}')
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-LOGS.png'))
    plt.clf()
    
    rounds_mean = df[df['round'] <= df['round'].max()/2]['agr_rounds'].mean()
    dec_mean = df[df['round'] <= df['round'].max()/2]['decided'].mean()
    re_mean = df[df['round'] <= df['round'].max()/2]['restarts'].mean()
    sns.lineplot(data=df, x='round', y='agr_rounds')
    sns.lineplot(data=df, x='round', y='decided')
    sns.lineplot(data=df, x='round', y='restarts')
    plt.title(f'mean dec: {dec_mean:.0f}, mean rnds: {rounds_mean:.0f}, mean restarts: {re_mean:.0f}')
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-AGRMNT.png'))
    plt.clf()
    
    sns.lineplot(data=df[df['pid'] <= 10], x='round', y='loaded_tail', hue='pid', palette='deep')
    sns.lineplot(data=df[df['pid'] <= 10], x='round', y='active_tail', hue='pid', palette='pastel', legend=False)
    sns.lineplot(data=df[df['pid'] <= 10], x='round', y='active_front', hue='pid', palette='pastel', legend=False)
    sns.lineplot(data=df[df['pid'] <= 10], x='round', y='loaded_front', hue='pid', palette='deep', legend=False)
    plt.title('window')
    plt.savefig(os.path.join(out_dir, f'{p}-{n}-{vs}-{ds}-WINDOW.png'))
    # plt.show()
    plt.clf()


