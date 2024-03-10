%% Mobile Robot Model Predictive Controller for Trajectory Following
% L Parrish March 10, 2024
clear; clc; close all;

%% MPC Controller Creation and Setup
% Set Number of States (nx), number of outputs (ny), and inputs or Manipulated Variables (nu)
nx = 3;
ny = 3; % Assuming full state feedback
nu = 2;

% Create a non-linear MPC controller
MPCController = nlmpc(nx,ny,nu);

% Set Discrete Sampling Time in seconds
Ts = 0.1;
MPCController.Ts = Ts;

% Set Prediction Horizon and Control Horizon
MPCController.PredictionHorizon = 10;
MPCController.ControlHorizon = 5;

% Set the discrete time system model for Unicycle Robot
MPCController.Model.StateFcn = "UnicycleDiscrete";
MPCController.Model.IsContinuousTime = false;
MPCController.Model.NumberOfParameters = 1;

% Set the system output model for the unicycle robot
MPCController.Model.OutputFcn = "UnicycleOutput";

% Define MPC Cost
MPCController.Weights.OutputVariables = [10 10 0]; % State Error Cost
MPCController.Weights.ManipulatedVariables = [1 1]; % Input Cost
MPCController.Weights.ManipulatedVariablesRate = [0.1 0.1]; % Input Rate Cost

% Define Constraint on State Variables
MPCController.OV(1).Min = -10;
MPCController.OV(1).Max = 10;

MPCController.OV(2).Min = -10;
MPCController.OV(2).Max = 10;

% Define Constraints on Actuator Inputs
MPCController.MV(1).Min = -1.0;
MPCController.MV(1).Max = 2.0;

MPCController.MV(2).Min = -1.0;
MPCController.MV(2).Max = 1.0;

% Define Constraints on Actuator Input Rate of Change
MPCController.MV(1).RateMin = -0.1;
MPCController.MV(1).RateMax = 0.1;

MPCController.MV(2).RateMin = -0.3;
MPCController.MV(2).RateMax = 0.3;

%% MPC Controller Validation
x0 = [0; -4; 0];
u0 = [0; 0];
validateFcns(MPCController,x0,u0,[],{Ts});

%% Create Reference Trajectory
tF = 60;
t = 0:Ts:tF;
xref = zeros(nx,length(t));
xref(1,:) = 4*sin(t/5);
xref(2,:) = 4*sin(t/5).*(1-cos(t/5));
xref = xref';

%% Simulate MPC Controller

options = nlmpcmoveopt;
options.Parameters = {Ts};
x = x0;
u = u0;
xHistory = x;
uHistory = u;
trajectorylookahead = 5;
xref = [xref; xref(end,:); xref(end,:); xref(end,:); xref(end,:); xref(end,:)];

for k = 1:length(t)
    % Update state reference
    ref = xref(k:k+trajectorylookahead,:);

    % Compute MPC
    u = nlmpcmove(MPCController,x,u,ref,[],options);

    % Apply optimal inputs and simulate next timestep
    x = UnicycleDiscrete(x,u,Ts);

    % Save States and Input Histories
    xHistory = [xHistory x];
    uHistory = [uHistory u];
end
xHistory(:,1) = [];
uHistory(:,1) = [];

%% Plot Results
figure;
Ylabels = {'x [m]','y [m]','\theta [rad]'};
for p = 1:nx
    subplot(nx,1,p);
    hold on;
    plot(t,xHistory(p,:));
    plot(t,xref(1:length(t),p),'r--');
    xlabel('Time [s]');
    ylabel(Ylabels(p));
    legend('State Variable','Reference');
end
sgtitle("State Variables");


figure;
Ylabels = {'Velocity [m/s]','\omega [rad/s]'};
for p = 1:nu
    subplot(nu,1,p);
    plot(t,uHistory(p,:));
    xlabel('Time [s]');
    ylabel(Ylabels(p));
end
sgtitle("Actuator Inputs");

figure;
hold on;
plot(xHistory(1,:),xHistory(2,:));
plot(xref(1:length(t),1),xref(1:length(t),2),'r--');
xlabel('x [m]');
ylabel('y [m]');
title('Robot Trajectory');
legend('Actual Trajectory','Reference Trajectory');