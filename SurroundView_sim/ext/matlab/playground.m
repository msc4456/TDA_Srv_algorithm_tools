[obj_v, obj_f] = fread_obj('../Rhino/20190421.obj');
obj_v = [obj_v(:,2),-obj_v(:,1),obj_v(:,3)];

% figure;
% hold on;
% for i = 1:size(obj_v,1)
%     plot3(obj_v(i,1),obj_v(i,2),obj_v(i,3),'ro');
% end
% for i = 1:size(obj_f,1)
%     p1 = obj_v(obj_f(i,1),:);
%     p2 = obj_v(obj_f(i,2),:);
%     p3 = obj_v(obj_f(i,3),:);
%     pp = [p1;p2;p3];
%     xx = pp(:,1)';
%     yy = pp(:,2)';
%     zz = pp(:,3)';
%     fill3(xx,yy,zz,[0,0,1]);
% %     pause;
%     pause(0.01);
% end

calib_data_front = load('20190414_guan_datong_front_0.mat');
calib_data_right = load('20190414_guan_datong_right_0.mat');
calib_data_back = load('20190414_guan_datong_back_0.mat');
calib_data_left = load('20190414_guan_datong_left_0.mat');

calib_data_front = calib_data_front.calib_data_front;
calib_data_right = calib_data_right.calib_data_right;
calib_data_back = calib_data_back.calib_data_back;
calib_data_left = calib_data_left.calib_data_left;

% Extrinsic  parameters
rrt_front = calib_data_front.RRfin;
rrt_right = calib_data_right.RRfin;
rrt_back = calib_data_back.RRfin;
rrt_left = calib_data_left.RRfin;

% Intrinsic parameters
ocam_model_front = calib_data_front.ocam_model;
ocam_model_right = calib_data_right.ocam_model;
ocam_model_back = calib_data_back.ocam_model;
ocam_model_left = calib_data_left.ocam_model;

ocam_model_front.pol = findinvpoly(ocam_model_front.ss,sqrt((ocam_model_front.width/2)^2+(ocam_model_front.height/2)^2));
ocam_model_right.pol = findinvpoly(ocam_model_right.ss,sqrt((ocam_model_right.width/2)^2+(ocam_model_right.height/2)^2));
ocam_model_back.pol = findinvpoly(ocam_model_back.ss,sqrt((ocam_model_back.width/2)^2+(ocam_model_back.height/2)^2));
ocam_model_left.pol = findinvpoly(ocam_model_left.ss,sqrt((ocam_model_left.width/2)^2+(ocam_model_left.height/2)^2));

% Load image
input_front = double(imread('20190414_guan_datong_front_0.bmp'))/255;
input_right = double(imread('20190414_guan_datong_right_0.bmp'))/255;
input_back = double(imread('20190414_guan_datong_back_0.bmp'))/255;
input_left = double(imread('20190414_guan_datong_left_0.bmp'))/255;

%%





%% function define

function [obj_v, obj_f] = fread_obj(fileName)
obj_v = zeros(50000,3);
obj_v_cnt = 0;
obj_f = zeros(50000,3);
obj_f_cnt = 0;

fileID = fopen(fileName,'r');
while true
    [flag,count] = fscanf(fileID,'%c',1);
    if count == 0
        break
    end
    switch flag
        case '#'
            fgetl(fileID);
        case 'v'
            [v,count] = fscanf(fileID,'%f',3);
            assert(count == 3);
            obj_v_cnt = obj_v_cnt+1;
            obj_v(obj_v_cnt,:) = v;
        case 'f'
            [f,count] = fscanf(fileID,'%f',3);
            assert(count == 3);
            obj_f_cnt = obj_f_cnt+1;
            obj_f(obj_f_cnt,:) = f;
    end
end

obj_v = obj_v(1:obj_v_cnt,:);
obj_f = obj_f(1:obj_f_cnt,:);

end