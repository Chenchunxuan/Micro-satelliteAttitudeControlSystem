clear;
N = 2000;
T = 0.005;

sigma_v = 6.9906*10^(-6);   %����Ư�������ķ���
sigma_n = 1.4142*10^(-9);   %����״̬���������������ķ���
sigma_x = 1*10^(-5);        %��������״̬�����ķ���
sigma_z = 1*10^(-12);       %�����������������ķ���

g_x=randn(3,N)*sigma_n;   %����״̬��������ֵΪ0������Ϊsigma_n
g_z=randn(3,N)*sigma_n;   %��������������������ֵΪ0������Ϊsigma_n
v = randn(3,N)*sigma_v;   %����Ư������

s_x=randn(4,N)*sigma_x;     %��������״̬��������ֵΪ0������Ϊsigma
s_z=randn(4,N)*sigma_z;     %��������������������ֵΪ0������Ϊsigma

X_star(:,1)=[1 0 0 0]';   %������������ֵ
X_gyro(:,1)=[0 0 0]';     %��������ֵ

gyro_drift = 0;           %����Ư�Ƴ�ֵ
for t=2:N
    X_star(:,t) = X_star(:,t-1);
    X_gyro(:,t) = X_gyro(:,t-1);
end
for i=1:N
    gyro_drift = gyro_drift+T*v(:,i);           %����Ư�ƻ���
    X_star_noise(:,i) = X_star(:,i)+s_x(:,i);   %ģ��ʵ����̬��
    Z_star(:,i) = X_star_noise(:,i)+s_z(:,i);   %ģ��������̬��
    X_gyro_noise(:,i) = X_gyro(:,i)+g_x(:,i);   %ģ��ʵ����̬���ٶ�
    Z_gyro(:,i) = X_gyro_noise(:,i)+g_z(:,i)+gyro_drift;   %ģ��������̬���ٶ�  
end

figure;
% plot(Z_star(1,:),'b')
% hold on;
plot(Z_star(2,:),'r')
hold on;
plot(Z_star(3,:),'y')
hold on;
plot(Z_star(4,:),'c')
hold on;
title('star');
legend()

figure;
plot(Z_gyro(1,:),'b')
hold on;
plot(Z_gyro(2,:),'r')
hold on;
plot(Z_gyro(3,:),'y')
hold on;
title('gyro');
legend()
