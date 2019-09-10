function Trg_indices = Gen_indices(gride_H,gride_W,render_mode)
%**********************生成三角形索引**************************************
% GL_TRIANGLE_STRIP mode
% V1— V3 — V5
%  | /   \  / ...
% V2  —  V4

% GL_TRIANGLE mode
% V1— V2   V2    V2 —V5
%  | /     /  \     \   / 
% V3     V3—  V4    V4

%if strcmp(render_mode ,'GL_TRIANGLE')
    triangle_num =0;
    for i=1:gride_H
        for j=1:gride_W
        v1 =(i-1)*(gride_W+1)+j;
        v3 = i*(gride_W+1)+j;
         %上三角形
        triangle_num = triangle_num+1;
        Trg_indices(triangle_num,1:3) =[v1,v1+1,v3]; 
        %下三角形
        triangle_num = triangle_num+1;
        Trg_indices(triangle_num,1:3) =[v1+1,v3+1,v3]; 
        end
    end
% end


