# IIR First-Order Low-Pass Filter on Short Integers

Minimal portable implementation of an IIR (Infinite Impulse Response) first order low-pass filter on `short` integers (signed: [-32768; 32767])

Implies a `divider` which is a positive non-null integer which defines the 1/n proportion of new input:

- output(n) = input(n) / n + output(n-1) x (n-1) / n

Calculation are done on integer only, intended for fast calculations. Anyway it implies one integer division (with rounding) for each input of a new value as well as for the reading of the filter output

*Side note*: if `divider` is a power of 2 (1, 2, 4, 8...), a right binary shift (`>>`) of *log.2*(`divider`) could be used instead to further speed up calculations (with right shifts anyhow, remember their behavior on *negative* numbers is compiler dependent and consequently should be avoided)

Though working on integers this filter, given a constant input will always converge to this value after a variable number of repeated inputs

## Performance

We can evaluate the performance of this filter in terms of time constants *tau* which expresses how close we are to a constant input after a repetition of the same value over and over

Here is the percentage of the final value we can get after some time constant multiples [given by the formula *1 - e^(-tau)*]:

|*tau*|1|2|3|4|5|6|7|8|9|10|
|---|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|%|63.212|86.466|95.021|98.168|99.326|99.752|99.909|99.966|99.988|99.995|

And below is the maximum number of input repeats it takes to reach a number of *tau*'s and the full convergence (100%) versus `divider`: 

|div/tau|1|2|3|4|5|6|7|8|9|10|converge|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|---|
|1|1|1|1|1|1|1|1|1|1|1|1|
|2|3|4|6|7|9|10|12|13|14|16|17|
|3|5|7|10|12|15|17|20|22|24|27|29|
|4|6|10|13|17|20|24|27|31|34|37|41|
|5|8|13|17|22|26|30|35|40|43|48|53|
|6|10|15|21|26|32|37|43|48|53|59|65|
|7|11|18|24|31|37|44|50|57|63|70|76|
|8|13|21|28|36|43|51|58|66|72|80|88|
|9|15|23|32|40|49|57|66|75|82|91|99|
|10|17|26|36|45|55|64|74|83|91|102|111|
|11|18|29|39|50|60|71|81|92|101|113|123|
|12|20|31|43|54|66|77|89|101|111|123|135|
|13|22|34|47|59|72|84|97|110|120|134|146|
|14|23|37|50|64|77|91|104|118|130|145|158|
|15|25|40|54|69|83|97|112|127|139|155|170|
|16|27|42|58|73|89|104|120|136|149|166|182|


This table was computed for an initial value of 32,767 to a final value of -32,768, which represents the maximum amplitude. A smaller amplitude makes the filter converge more rapidly

# Test Program

The test program prints the two tables given in the previous section, (values separated by tabs `\t`)

It can be easily recompiled and run with different `initial`, `final` values as well as `divider` and `tau` ranges in the nested `for` loops