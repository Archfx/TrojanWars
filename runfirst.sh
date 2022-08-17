export midroot="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
export flatten="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/flatten/flatten"
export rare="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/rareanalyser/rareAnalysis"
export ndetect="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/ndetect/ndetect"
export ndatpg="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/ndatpg/ndatpg"
export tarmac="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/tarmac"
export mcatpg="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/mcatpg"
export techmap="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/Libraries/tech"
export synth="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/flatten/synth"
export coverage="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/coverage/coverage"
export midetect="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )/implantDetect/implant_detect"

echo $'\n ****** PATH variables added to the current Shell instance ******* \n'

source /apps/settings

echo $'\n ****** Modules loaded to the current Shell instance ******* \n'
