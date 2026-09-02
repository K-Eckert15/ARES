Er = 4.5        %Dielectric Constant
f0 = 2.4e9      %Design Frequency
h = 3.937e-3      %mil
C = 3e8     %SOL

W = C / (2*2.4e9 *sqrt((4.5+1)/2))
Eeff = ((Er+1)/2)+((Er-1)/2)*(1/(sqrt(1+12*((h)/W))))

(((Eeff+.3)*(W/h+.264)) / ((Eeff-.258)*(W/h+.8)))
L = (C/(2*f0*sqrt(Eeff))) - .824*h * (((Eeff+.3)*(W/h+.264)) / ((Eeff-.258)*(W/h+.8)))


fprintf('Width (W):                  %.2f mm\n', W * 1e3);
fprintf('Effective Dielectric (Eeff): %.4f\n', Eeff);
fprintf('Length (L):                 %.2f mm\n', L * 1e3);

