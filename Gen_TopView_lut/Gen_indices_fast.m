function Trg_indices = Gen_indices_fast(points,gride_H,gride_W)

%**********************三角形顶点排列顺序,顺时针排列*************************************
% GL_TRIANGLE_STRIP mode
% V1  V3   V5
%  | /   \  / ...
% V2     V4

% GL_TRIANGLES mode
% V1  V2    V2    V2   V5
%  | /     /  \    \  / 
% V3     V3   V4    V4

Trg_indices =[];

for i = 1: gride_H
    start = (i-1)*(gride_W+1)+1;
    v1 = points(start:start+gride_W-1);%1~n-1
    v2 = v1+1;
    start = i*(gride_W+1)+1;
    v3 = points(start:start+gride_W-1);
    v4 = v3+1;
    Trg_indices1(:,1:3) =[v1,v2,v3];
    Trg_indices2(:,1:3) =[v2,v4,v3];
    Trg_indices =[Trg_indices;Trg_indices1;Trg_indices2];
end
% Trg_indices(Trg_indices(:,))

