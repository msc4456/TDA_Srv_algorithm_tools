%算法思路，使用一组表格，四组索引进行传入GLSL
%表格数据结构 [x,y,z,u1,v1,u2,v2,blendvalus1,blendvalus2]
%着色器每次传入2幅纹理，
function [vertex,num_v,surface,num_f] = gen_srv_lut_from3Dfile(src_path,scaler)
% clc
% clear all;
% src_path ='wall.obj';
% EN_DRAW  =0;
% scaler   = [1.25 ,1,1];


%**************************从3D模型中读入顶点和曲面信息*************************
file_id = fopen(src_path,'r');
if file_id == -1
    disp('There NO .pbj file at the path');
    return
end
num_v =0;
num_f =0;
while feof(file_id)==0         %文件未读取到结尾
     line = fgetl(file_id);
     if strcmp(line(1:3),'v  ')%空格数量根据导出的文件进行调整
         num_v =num_v + 1;
         line = strrep(line,' ',',');
%          fprintf(temp_id,'%s\n',line(4:end)); 
         vertex(num_v,:) = str2num(line(4:end)); %转换过程中会出现精度损失
%          disp(num_v);
     end
     if strcmp(line(1:2),'f ')==1
         num_f =num_f + 1;
         line = strrep(line,'/',',');
         line = strrep(line,' ',';');
%          fprintf(temp_id,'%s\n',line(3:end));
         surface(num_f,1:3) = str2num(line(3:end));%提取曲面顶点信息，首列为顶点顺序
%          disp(num_f);
     end
end
fclose(file_id);

disp('finished import srv_lut from 3dfiles');

%*******************************输出曲面数组*********************************
%世界坐标尺度因子--由3DMAX建模引入了尺度比例与真实比例不一致的情况，通过此处进行调整，
%默认比例为10，即3D文件的绘图单位为cm

vertex =vertex*[scaler(1),0,0;0,scaler(2),0;0,0,scaler(3)];          

temp_size = size(vertex);
for i=1:temp_size(1)
    vertex(i,1:4) = [i,vertex(i,1:3)];
end

% %*******************************曲面分割************************************
% 
% [mask_vertex,index_quad] = quard_segment(vertex,combine_dist,surface,num_f);




