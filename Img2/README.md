# Laiks
Laiks bija mākoņains, bet arī mainīgs gaismas apjomā. Bija ļoti vējains, kas izraisīja drona kustību un krūmu kustību, tādēļ attēlā ir daudz artifaktu un nepareizi salipināti krūmi.

# Rezultāti

Rezultāti ir gandrīz lietojami - attēls neatbilst realitātei, tādēļ nav iespējams iegūt pareizu rezultātu, bet daļās, kur attēls ir daudz maz izdevies, iegūtie rezultāti ir daudz maz labi.

Eksperimentālais indekss ir ievērojami labāks nekā Excess Green indekss, bet salīdzinot ar Img1, kreisajā malā krūmu pikseļi ir pazuduši. Tas visdrīzāk ir dēļ mainīgā gaismas apjoma, jo pašas kreisās četras rindas vēl ir saprotams, ja pazustu, jo tā ir cita šķirne un to var ļoti labi redzēt krāsā, bet ir pazudušas arī blakus rindas, ka liecina par problēmu attēlā uzņemtajā apgabalā.

Gan interpolēto, gan iepriekšējo daļu punkti ir pielietoti ļoti veiksmīgi. Vienīgais labajā malā ir redzams, kas var notikt, ja tiek izvēlēts nepareizs punkts (šajā gadījumā tās ir attēlu neveiksmīgas salipināšanas sekas) - ir iespējams, ka algoritms neatkopsies un turpinās šo nepareizo punktu tālāk, kas sabojās visus rezultātus.

## Oriģinālais attēls
![Oriģinālais attēls](https://github.com/askolds11/dronerobotroute/blob/assets/Img2/Img2.jpg?raw=true)

## Otsu rezultāts
![Otsu rezultāts](https://github.com/askolds11/dronerobotroute/blob/assets/Img2/2_Otsu_3.jpg?raw=true)

## Rindas
![Rindas](https://github.com/askolds11/dronerobotroute/blob/assets/Img2/4_Result_Rows.jpg?raw=true)

## Rindstarpas
![Rindstarpas](https://github.com/askolds11/dronerobotroute/blob/assets/Img2/4_Result_BetweenRows.jpg?raw=true)

## Grafs
![Grafs](https://github.com/askolds11/dronerobotroute/blob/assets/Img2/4_Result_Graph.jpg?raw=true)