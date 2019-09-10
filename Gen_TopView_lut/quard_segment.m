%对曲面分割成 Q1 Q2 Q3 Q4 四个单元
function [mask_vertex_out,index_quad]  = quard_segment(mask_vertex,combine_dist,surface,num_f)
%mask_vertex(x,y) :输入的曲面三维坐标顶点
%mask_vertex_out  :区域划分后的输出
%          |
%    Q1    |    Q2
%__________|__________
%          |
%    Q4    |    Q3
%          |
%combine_dist 保持一定的交跌区域，防止出现空洞区域

mask_vertex_out(:,2:5) = mask_vertex; 


%初始化通道曲面数组
index_quad1   = []; index_quad2   = [];index_quad3   = []; index_quad4   = [];


% Quard 1
ind = find(mask_vertex(:,2)<=combine_dist & mask_vertex(:,3)>=-combine_dist);
mask_vertex_out(ind,2:5) = mask_vertex(ind,1:4);
mask_vertex_out(ind,1) = 1;

% for i =1:num_f
%     if(mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,2),1)&&...
%        mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,3),1)&&...
%        mask_vertex_out(surface(i,1),1)== 1)
%        index_quad1 =[index_quad1;surface(i,1:3)]; %属于quad1的三角形
%     end
% end

% Quard 2
ind = find(mask_vertex(:,2)>=-combine_dist & mask_vertex(:,3)>=-combine_dist);
mask_vertex_out(ind,2:5) = mask_vertex(ind,1:4);
mask_vertex_out(ind,1) = 2;

% for i =1:num_f
%     if(mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,2),1)&&...
%        mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,3),1)&&...
%        mask_vertex_out(surface(i,1),1)== 2)
%        index_quad2 =[index_quad2;surface(i,1:3)]; %属于quad2的三角形
%     end
% end

% Quard 3
ind = find(mask_vertex(:,2)>=-combine_dist & mask_vertex(:,3)<=combine_dist);
mask_vertex_out(ind,2:5) = mask_vertex(ind,1:4);
mask_vertex_out(ind,1) = 3;

% for i =1:num_f
%     if(mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,2),1)&&...
%        mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,3),1)&&...
%        mask_vertex_out(surface(i,1),1)== 3)
%        index_quad3 =[index_quad3;surface(i,1:3)]; %属于quad3的三角形
%     end
% end

% Quard 4
ind = find(mask_vertex(:,2)<= combine_dist & mask_vertex(:,3)<= combine_dist);
mask_vertex_out(ind,2:5) = mask_vertex(ind,1:4);
mask_vertex_out(ind,1) = 4;


% for i =1:num_f
%     if(mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,2),1)&&...
%        mask_vertex_out(surface(i,1),1)==mask_vertex_out(surface(i,3),1)&&...
%        mask_vertex_out(surface(i,1),1)== 4)
%        index_quad4 =[index_quad4;surface(i,1:3)]; %属于quad4的三角形
%     end
% end



%对曲面的顶点进行判断，如果三角形三个顶点全落在一个通道的相机视野中
%则将该三角形片元分配到该通道中


temp_size = size(index_quad1);
index_quad1 =reshape(index_quad1',temp_size(1)*temp_size(2),1);

temp_size = size(index_quad2);
index_quad2 =reshape(index_quad2',temp_size(1)*temp_size(2),1);

temp_size = size(index_quad3);
index_quad3 =reshape(index_quad3',temp_size(1)*temp_size(2),1);

temp_size = size(index_quad4);
index_quad4 =reshape(index_quad4',temp_size(1)*temp_size(2),1);

index_quad = {index_quad1,index_quad2,index_quad3,index_quad4};