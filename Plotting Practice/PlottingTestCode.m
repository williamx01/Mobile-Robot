clc; clear; close all;
%% Control Plot
xPos = 20;
yPos = 10;
theta = pi/2;

x = [1,2,3,4,5];
y = [1,2,3,4,5];
z = [2,4,6,8,10];
y1 = [1,2,3,4,5];
z1 = [2,4,6,8,10];
hold on
plot(x, y,'.', 'MarkerSize',12,'Color',[0 0.4470 0.7410],'DisplayName','\omega_{L} [rad/s]')
hold on
plot(x,z,'.', 'MarkerSize',12,'Color',[0.6350 0.0780 0.1840],'DisplayName','\omega_{L} [rad/s]')
legend('Location','Southeast')

hold off
%% State Plot
x = [1,2,3,4,5];
y = [1,2,3,4,5];
z = [2,4,6,8,10];
d = [3,6,9,12,15];
y1 = [1,2,3,4,5];
z1 = [2,4,6,8,10];
d1 = [3,6,9,12,15];  


hold on
plot(x, y,'.', 'MarkerSize',12,'Color',[0 0.4470 0.7410],'DisplayName','x [m]')
hold on
plot(x,z,'.', 'MarkerSize',12,'Color',[0.6350 0.0780 0.1840],'DisplayName','y [m]')
hold on
plot(x,d,'.','MarkerSize',12,'Color',[0.9290 0.6940 0.1250],'DisplayName','\theta [rad]')
legend('Location','Southeast')

%% Trajectory Plot
fill(x,y, 'r')
hold on
fill(x,z,'g')
hold on
plot(x,z1,'b--')
hold on
fill(x,d,'b')

legend(["Starting Pose","Planned Pose","Planned Trajectory","Actual Trajectory"])

%% Append matrix
clear all
B = [1,2,3,4,5,6,7;1,2,3,4,5,6,7];
rows = size(B,1);
columns = size(B,2);

A = zeros(rows,columns);
empty = [0;0];
A = [A,empty];
disp(A)
