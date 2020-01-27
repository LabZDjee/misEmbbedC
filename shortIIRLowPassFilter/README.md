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
|1|2|2|2|2|2|2|2|2|2|2|2|
|2|3|4|5|7|8|10|11|13|14|15|17|
|3|4|6|9|11|14|16|19|21|23|26|28|
|4|5|8|12|15|19|22|26|29|32|36|40|
|5|6|10|15|19|24|28|33|37|41|46|51|
|6|7|12|18|23|29|34|40|45|50|56|62|
|7|8|14|21|27|34|40|47|54|59|66|72|
|8|9|16|24|31|39|46|54|62|68|76|84|
|9|10|18|27|35|44|52|61|70|77|86|94|
|10|11|20|30|39|49|58|68|78|86|96|106|
|11|12|22|33|43|54|64|75|86|95|106|117|
|12|13|24|36|47|59|70|82|94|103|116|128|
|13|14|26|39|51|64|76|89|102|113|126|138|
|14|15|28|42|55|69|82|96|110|121|136|149|
|15|16|30|45|59|74|88|103|118|130|146|161|
|16|17|32|48|63|79|94|110|126|139|156|172|

This table was computed for an initial value of 32,767 to a final value of -32,768, which represents the maximum amplitude. A smaller amplitude makes the filter converge more rapidly

# Test Program

The test program prints the two tables given in the previous section, (values separated by tabs `\t`)

It can be easily recompiled and run with different `initial`, `final` values as well as `divider` and `tau` ranges in the nested `for` loops