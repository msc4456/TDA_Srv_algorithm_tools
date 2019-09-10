function [ocam_model]=get_Calibrate(calibrate_para_path)
calibrate = C_calib_data;
ocam_model = calibrate.ocam_model;
%**************************************************************************
%                        从文本文件中读取参数信息
%**************************************************************************
file_id =fopen(calibrate_para_path,'r');
if file_id == -1
    disp('NO calibration file was found,undistortion stopped');
    return
end
disp('runing undistortion,reading calibration data...');
% type calibrate_para_path;

fscanf(file_id,'\n'); 
MAXCHAR_BUFFER = 1024;
fgets(file_id,MAXCHAR_BUFFER);
ss_temp = fscanf(file_id,'%f');
ocam_model.ss = ss_temp';

MAXCHAR_BUFFER = 1024;
fgets(file_id,MAXCHAR_BUFFER);
ss_temp = fscanf(file_id,'%f');
ocam_model.pol = ss_temp';

MAXCHAR_BUFFER = 1024;
fgets(file_id,MAXCHAR_BUFFER);
center = fscanf(file_id,'%f');
ocam_model.xc = center(1,1);
ocam_model.yc = center(2,1);
% ocam_model.xc = 360;
% ocam_model.yc = 640;

MAXCHAR_BUFFER = 1024;
fgets(file_id,MAXCHAR_BUFFER);
affine = fscanf(file_id,'%f');
ocam_model.c = affine(1,1);
ocam_model.d = affine(2,1);
ocam_model.e = affine(3,1);


MAXCHAR_BUFFER = 1024;
fgets(file_id,MAXCHAR_BUFFER);
size_temp = fscanf(file_id,'%f');
ocam_model.height  = size_temp(1,1);
ocam_model.width   = size_temp(2,1);

fclose(file_id);
end

