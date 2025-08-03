
## Dress Me Up

### Overview


<img src="https://github.com/Shtrompel/BGal256ModulesDocs/blob/main/SortStep.png?raw=true" style="width:50%;">

**SortStep** is a sequencer that takes convert the steps needed to sort an array to CV, based on the famous [array sorting visualizations videos](https://www.youtube.com/watch?v=kPRA0W1kECg). For every operation the index of the operation and the value of the operated element of the array are outputted at **(15)** and **(16)** respectively. Some operation require access to to elements at the same time so the output will have two channels for each element.

The sorting of the array is divided into three stages:
**Shuffle** - Randomize the array using the [_Fisher-Yates shuffle_ algorithm](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle). Each stage of the shuffling will be outputted the same way sorting operations are outputted.
**Sort** - According to the chosen sorting algorithm, the array will be sorted.
**Traverse**: After the array is sorted, traverse the sorted array from the left and export the index/value of the current index of the traversal.

Additional setting can be accessed from the screen by pressing the **(18)** to **(20)** buttons.

## Inputs

| No. | Input             | Description                                 | Signal Type |
| --- | ----------------- | ------------------------------------------- | ----------- |
| 1   | Global Step        | Do a single operation, and if the current stage is finished go to the next stage. The order is **Shuffle** -> **Sort** -> **Traverse**. If **Traverse** was finished, no more operations will be processed.            | Trigger     |
| 2   | Shuffle Step       | Do a single [_Fisher-Yates shuffle_ algorithm](https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle) operation. Once the algorithm is finished it will be repeated from the start.         | Trigger     |
| 3   | Sort Step       | Do a single sorting operation using the currently selected sorting algorithm. Once the algorithm is finished no more operations will be processed.  | Trigger     |
| 4   | Traverse Step      | Move to the next value in the traversal. Once the traversal is finished it will be reset from the left.    | Trigger |
| 8   | Algorithm Select        | Select the currently used algorithm using voltage. For manual selection see the menu at **(18)**. |  0-10 V  |
| 9   | Array Size           | Change the length of the array. For manual selection see the menu at **(18)**.         |  0-10 V  |

## Outputs

| No. | Output             | Description                              | Signal Type |
| --- | ------------------ | ---------------------------------------- | ----------- |
| 11  | Global On End  | After all operation within the three stages were done, a trigger will be outputted here. | Trigger     |
| 12  | Shuffle On End | Once the shuffling algorithm shuffled the entire array, a trigger will be outputted here.          | Trigger     |
| 13  | Sort On End | Once the sorting algorithm sorted the entire array, a trigger will be outputted here.          | Trigger     |
| 14  | Traverse On End | Once the traversing stage went through the entire array, a trigger will be outputted here.          | Trigger     |
| 15  | Value Out | For every array operation, the value of the accessed element will be normalized and outputted here. If two elements were operated upon two channels will be outputted.          | Range depends on  **(17)**. Default is 0 - 1 V.     |
| 16  | Index Out | For every array operation, the index of the accessed element will be normalized and outputted here. If two elements were operated upon two channels will be outputted.          | Range depends on  **(17)**. Default is 0 - 1 V.     |

## Knobs & Switches

| No. | Control        | Description                                            | Range             |
| --- | -------------- | ------------------------------------------------------ | ----------------- |
| 17  | Out Scale            | Change the output range of the **Value Out** **(15)** and **Index Out** **(16)**. The scale will be from 0 to the value of the value of the param.                              | 0 - 10         |

## UI Menu
| No. | Control        | Description                                            | Type             |
| - | - | - | - |
| 18  | Show/Hide Menu            | Open the [generic settings menu](#generic-menu)                             | Button         |
| 19  | Show/Hide Filters            | Open the [operation filtering settings menu](#filters-menu)                             | Button         |
| 20  | Show/Hide Key Out            | Open the [scale key out settings menu](#key-out-menu)                             | Button         |

## Generic Menu

<img src="https://github.com/Shtrompel/BGal256ModulesDocs/blob/main/SortStepMenu.png?raw=true" style="width:25%;">

| No. | Control        | Description                                            | Type             |
| - | - | - | - |
| 21  | Array Size            | Changes the number of elements within the array.                            | 1 - 1000 Elements         |
| 22  | Change Algorithm            | Changes the type of the sorting algorithm, the currently used algorithm is shown below this button.                             | Context Menu Selection         |
| 23  | Shuffle Skips            | When in the **Shuffle** stage,                              | 1 - 256 Steps         |
| 24  | Traverse Skips            | Open the scaled key out settings menu.                             | 1 - 256 Steps         |

## Filters Menu

<img src="https://github.com/Shtrompel/BGal256ModulesDocs/blob/main/SortStepMenuFilter.png?raw=true" style="width:25%;">

25: 
When sorting, every operation is outputted to the value or index outputs. In the filtering menu you can choose to skip any type of operation. Once disabled when the algorithm will do all of the filtered operations in the background and only output the closest non filtered operation.            Changes the number of elements within the array.    
The operations are: 
* Cmp - Compare between two elements, this operation does not do any changes.
* Swap - Swap between the values of two indexes.
* Move - Take the value from one index and move it to another index.
* Set - Change the value of an element within an index.
* Read - Read the value from an index. This operation does not do any changes.
* Remove - Remove the element from the array.

                       

## Key Out Menu

<img src="https://github.com/Shtrompel/BGal256ModulesDocs/blob/main/SortStepMenuKeyOut.png?raw=true" style="width:25%;">

| No. | Control        | Description                                            | Type             |
| - | - | - | - |
| 26  | Scale Disabled/Enabled           | When disabled, the index or element value is outputted in the normalized range from 0 to the value of the **Scale** knob **(17)**. When scale output is enabled every value is mapped unto a key within a scale instead. If the value of the key becomes bigger than 10 it will output 10.                           | Toggle        |
| 27  | Change Scale            | Choose the scale for the output.                             | Context Menu Selection         |
| 28  | Key Offset            | Offset the output key.                             | -24  - 24         |

